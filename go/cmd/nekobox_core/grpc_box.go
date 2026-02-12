package main

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"reflect"

	"grpc_server"
	"grpc_server/gen"

	"github.com/matsuridayo/libneko/neko_common"
	"github.com/matsuridayo/libneko/neko_log"
	"github.com/matsuridayo/libneko/speedtest"
	box "github.com/sagernet/sing-box"
	"github.com/sagernet/sing-box/boxapi"
	boxmain "github.com/sagernet/sing-box/cmd/sing-box"

	"log"

	"github.com/sagernet/sing-box/option"
)

type server struct {
	grpc_server.BaseServer
}

func (s *server) Start(ctx context.Context, in *gen.LoadConfigReq) (out *gen.ErrorResp, _ error) {
	var err error

	defer func() {
		out = &gen.ErrorResp{}
		if err != nil {
			out.Error = err.Error()
			instance = nil
		}
	}()

	if neko_common.Debug {
		log.Println("Start:", in.CoreConfig)
	}

	if instance != nil {
		err = errors.New("instance already started")
		return
	}

	instance, instance_cancel, err = boxmain.Create([]byte(in.CoreConfig))

	if instance != nil {
		// Logger
		instance.SetLogWritter(neko_log.LogWriter)
		// V2ray Service
		if in.StatsOutbounds != nil {
			statsOpt := option.V2RayStatsServiceOptions{
				Enabled:   true,
				Outbounds: in.StatsOutbounds,
			}
			enableNekorayConnectionsCompat(&statsOpt, in.GetEnableNekorayConnections())
			instance.Router().SetV2RayServer(boxapi.NewSbV2rayServer(statsOpt))
		}
	}

	return
}

func (s *server) Stop(ctx context.Context, in *gen.EmptyReq) (out *gen.ErrorResp, _ error) {
	var err error

	defer func() {
		out = &gen.ErrorResp{}
		if err != nil {
			out.Error = err.Error()
		}
	}()

	if instance == nil {
		return
	}

	instance_cancel()
	instance.Close()

	instance = nil

	return
}

func (s *server) Test(ctx context.Context, in *gen.TestReq) (out *gen.TestResp, _ error) {
	var err error
	out = &gen.TestResp{Ms: 0}

	defer func() {
		if err != nil {
			out.Error = err.Error()
		}
	}()

	if in.Mode == gen.TestMode_UrlTest {
		var i *box.Box
		var cancel context.CancelFunc
		if in.Config != nil {
			// Test instance
			i, cancel, err = boxmain.Create([]byte(in.Config.CoreConfig))
			if i != nil {
				defer i.Close()
				defer cancel()
			}
			if err != nil {
				return
			}
		} else {
			// Test running instance
			i = instance
			if i == nil {
				return
			}
		}
		// Latency
		out.Ms, err = speedtest.UrlTest(boxapi.CreateProxyHttpClient(i), in.Url, in.Timeout, speedtest.UrlTestStandard_RTT)
	} else if in.Mode == gen.TestMode_TcpPing {
		out.Ms, err = speedtest.TcpPing(in.Address, in.Timeout)
	} else if in.Mode == gen.TestMode_FullTest {
		i, cancel, err := boxmain.Create([]byte(in.Config.CoreConfig))
		if i != nil {
			defer i.Close()
			defer cancel()
		}
		if err != nil {
			return
		}
		return grpc_server.DoFullTest(ctx, in, i)
	}

	return
}

func (s *server) QueryStats(ctx context.Context, in *gen.QueryStatsReq) (out *gen.QueryStatsResp, _ error) {
	out = &gen.QueryStatsResp{}

	if instance != nil {
		if ss, ok := instance.Router().V2RayServer().(*boxapi.SbV2rayServer); ok {
			out.Traffic = ss.QueryStats(fmt.Sprintf("outbound>>>%s>>>traffic>>>%s", in.Tag, in.Direct))
		}
	}

	return
}

func (s *server) ListConnections(ctx context.Context, in *gen.EmptyReq) (*gen.ListConnectionsResp, error) {
	out := &gen.ListConnectionsResp{
		NekorayConnectionsJson: "[]",
	}
	if instance == nil {
		return out, nil
	}
	v2rayServer := instance.Router().V2RayServer()
	if v2rayServer == nil {
		return out, nil
	}

	jsonPayload := listConnectionsCompat(v2rayServer)
	if jsonPayload != "" {
		out.NekorayConnectionsJson = jsonPayload
	}
	return out, nil
}

func enableNekorayConnectionsCompat(statsOpt *option.V2RayStatsServiceOptions, enabled bool) {
	// Keep source compatible with multiple sing-box versions:
	// older versions do not have this field.
	v := reflect.ValueOf(statsOpt).Elem()
	for _, fieldName := range []string{
		"EnableNekorayConnections",
		"NekorayConnections",
		"EnableConnections",
		"Connections",
	} {
		f := v.FieldByName(fieldName)
		if f.IsValid() && f.CanSet() && f.Kind() == reflect.Bool {
			f.SetBool(enabled)
			return
		}
	}
}

func listConnectionsCompat(v2rayServer any) string {
	// Probe common method names in different sing-box/libneko versions.
	for _, methodName := range []string{
		"ListConnections",
		"Connections",
		"GetConnections",
		"NekorayConnections",
		"DumpConnections",
	} {
		if payload, ok := callConnectionMethod(v2rayServer, methodName); ok {
			return payload
		}
	}
	return "[]"
}

func callConnectionMethod(target any, methodName string) (string, bool) {
	defer func() {
		_ = recover()
	}()

	mv := reflect.ValueOf(target).MethodByName(methodName)
	if !mv.IsValid() || mv.Type().NumIn() != 0 {
		return "", false
	}

	outs := mv.Call(nil)
	if len(outs) == 0 {
		return "", false
	}
	if len(outs) == 2 && !outs[1].IsNil() {
		return "", false
	}
	return normalizeConnectionsOutput(outs[0].Interface())
}

func normalizeConnectionsOutput(raw any) (string, bool) {
	switch v := raw.(type) {
	case string:
		return normalizeConnectionsJSON(v)
	case []byte:
		return normalizeConnectionsJSON(string(v))
	default:
		j, err := json.Marshal(v)
		if err != nil {
			return "", false
		}
		return normalizeConnectionsJSON(string(j))
	}
}

func normalizeConnectionsJSON(payload string) (string, bool) {
	if payload == "" {
		return "[]", true
	}

	var parsed any
	if err := json.Unmarshal([]byte(payload), &parsed); err != nil {
		return "", false
	}

	switch v := parsed.(type) {
	case []any:
		out, _ := json.Marshal(v)
		return string(out), true
	case map[string]any:
		for _, key := range []string{"connections", "Connections", "nekoray_connections", "data"} {
			if arr, ok := v[key].([]any); ok {
				out, _ := json.Marshal(arr)
				return string(out), true
			}
		}
	}

	return "[]", true
}
