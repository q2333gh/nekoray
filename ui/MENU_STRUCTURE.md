# MainWindow Menu Structure Documentation

This document describes the menu structure for `mainwindow.ui` to improve maintainability.

## Menu Organization

### Program Menu (`menu_program`)
- **Show Window** (`actionShow_window`)
- **Add profile from clipboard** (`menu_add_from_clipboard2`)
- **Scan QR Code** (`menu_scan_qr`)
- ---
- **Start with system** (`actionStart_with_system`) [checkable]
- **Remember last profile** (`actionRemember_last_proxy`) [checkable]
- **Allow other devices to connect** (`actionAllow_LAN`) [checkable]
- **Active Server** (`menuActive_Server`) [submenu, populated dynamically]
- **Active Routing** (`menuActive_Routing`) [submenu, populated dynamically]
- **System Proxy** (`menu_spmode`) [submenu]
  - Enable System Proxy (`menu_spmode_system_proxy`) [checkable]
  - Enable Tun (`menu_spmode_vpn`) [checkable]
  - Disable (`menu_spmode_disabled`) [checkable]
- **Preferences** (`menu_program_preference`) [submenu, contains `menu_preferences` actions]
- ---
- **Restart Proxy** (`actionRestart_Proxy`)
- **Restart Program** (`actionRestart_Program`)
- **Exit** (`menu_exit`)
- **Placeholder 1-3** (`actionPlaceholder1-3`) [disabled, for Windows tray menu bug fix]

### Preferences Menu (`menu_preferences`)
- **Groups** (`menu_manage_groups`)
- **Basic Settings** (`menu_basic_settings`)
- **Routing Settings** (`menu_routing_settings`)
- **Tun Settings** (`menu_vpn_settings`)
- **Hotkey Settings** (`menu_hotkey_settings`)
- **Open Config Folder** (`menu_open_config_folder`)

### Server Menu (`menu_server`)
- **New profile** (`menu_add_from_input`)
- **Add profile from clipboard** (`menu_add_from_clipboard`)
- ---
- **Start** (`menu_start`) [shortcut: Return]
- **Stop** (`menu_stop`) [shortcut: Ctrl+S]
- ---
- **Select All** (`menu_select_all`) [shortcut: Ctrl+A]
- **Move** (`menu_move`) [shortcut: Ctrl+M]
- **Clone** (`menu_clone`) [shortcut: Ctrl+D]
- **Reset Traffic** (`menu_reset_traffic`) [shortcut: Ctrl+R]
- **Delete** (`menu_delete`) [shortcut: Del]
- ---
- **Share** (`menu_share_item`) [submenu]
  - QR Code and link (`menu_qr`) [shortcut: Ctrl+Q]
  - Export %1 config (`menu_export_config`) [shortcut: Ctrl+E]
  - ---
  - Copy links of selected (`menu_copy_links`) [shortcut: Ctrl+C]
  - Copy links of selected (Neko Links) (`menu_copy_links_nkr`) [shortcut: Ctrl+N]
- ---
- **Current Select** (`menuCurrent_Select`) [submenu, populated dynamically]
- **Current Group** (`menuCurrent_Group`) [submenu]
  - Tcp Ping (`menu_tcp_ping`) [shortcut: Ctrl+Alt+T]
  - Url Test (`menu_url_test`) [shortcut: Ctrl+Alt+U]
  - Full Test (`menu_full_test`) [shortcut: Ctrl+Alt+F]
  - Stop Testing (`menu_stop_testing`) [shortcut: Ctrl+Alt+S]
  - Clear Test Result (`menu_clear_test_result`) [shortcut: Ctrl+Alt+C]
  - Resolve domain (`menu_resolve_domain`) [shortcut: Ctrl+Alt+I]
  - ---
  - Remove Unavailable (`menu_remove_unavailable`) [shortcut: Ctrl+Alt+R]
  - Remove Duplicates (`menu_delete_repeat`) [shortcut: Ctrl+Alt+D]
  - ---
  - Update subscription (`menu_update_subscription`) [shortcut: Ctrl+U]
- ---
- **Debug Info** (`menu_profile_debug_info`)
- ---

## Notes

- Menus marked as "populated dynamically" are filled at runtime based on current state
- Fake actions (`actionfake`, `actionfake_2`, etc.) are placeholders for dynamic content
- Placeholder actions at the end of Program menu fix Windows tray menu bug where Exit button gets cut off
- All shortcuts are defined in the action properties in the UI file

## Maintenance Tips

1. When adding new menu items, add the action definition in the `<action>` section
2. Add the action to the appropriate menu in the `<widget class="QMenu">` section
3. Update this documentation to reflect changes
4. Consider grouping related actions together for better organization
