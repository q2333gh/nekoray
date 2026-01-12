#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
文件大小分析工具 - 根据 file-size-limit 规范生成优化报告
"""

import os
import re
from pathlib import Path
from typing import List, Tuple, Dict
from dataclasses import dataclass

@dataclass
class FileAnalysis:
    path: str
    lines: int
    category: str
    reason: str
    suggestion: str

def count_code_lines(file_path: Path) -> int:
    """统计代码行数（不含空行和注释）"""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        
        code_lines = 0
        in_block_comment = False
        
        for line in lines:
            stripped = line.strip()
            
            # 跳过空行
            if not stripped:
                continue
            
            # 处理块注释
            if '/*' in stripped:
                in_block_comment = True
                # 检查是否在同一行结束
                if '*/' in stripped:
                    in_block_comment = False
                    # 检查注释后是否有代码
                    parts = stripped.split('*/')
                    if len(parts) > 1 and parts[1].strip():
                        code_lines += 1
                continue
            
            if in_block_comment:
                if '*/' in stripped:
                    in_block_comment = False
                    # 检查注释后是否有代码
                    parts = stripped.split('*/')
                    if len(parts) > 1 and parts[1].strip():
                        code_lines += 1
                continue
            
            # 跳过单行注释
            if stripped.startswith('//'):
                # 检查是否是 GENERATED 标记
                if 'GENERATED' in stripped:
                    return -1  # 标记为自动生成文件
                continue
            
            # 处理行内注释
            if '//' in stripped:
                code_part = stripped.split('//')[0].strip()
                if code_part:
                    code_lines += 1
                continue
            
            code_lines += 1
        
        return code_lines
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return 0

def analyze_file(file_path: Path, lines: int) -> FileAnalysis:
    """分析文件并分类"""
    rel_path = str(file_path.relative_to(Path.cwd()))
    
    # 自动生成文件跳过
    if lines == -1:
        return FileAnalysis(
            path=rel_path,
            lines=0,
            category="GENERATED",
            reason="自动生成文件，不受限制",
            suggestion="无需优化"
        )
    
    # 判断文件类型和分类
    path_str = str(file_path)
    
    # A类：允许接近600（450-600 OK），但超过600必须拆分
    # 1. 单窗口/单页面 UI 实现
    # 2. 核心进程控制器
    # 3. 单平台系统代理设置器
    # 4. 日志 Model / 解析适配器
    
    # B类：建议300-450
    # MainWindow.cpp / AppController.cpp
    # ConfigRepository.cpp
    # SystemIntegration 相关
    
    # C类：必须拆分（≥450基本就拆）
    # 多主题混杂、巨型分发逻辑、跨层依赖、函数过长
    
    category = ""
    reason = ""
    suggestion = ""
    
    if lines > 600:
        category = "必须拆分 (超过600行)"
        reason = f"文件超过600行限制（当前{lines}行）"
        suggestion = "必须立即拆分，不允许豁免"
    elif lines >= 450:
        # 判断是否属于A类
        is_ui_implementation = "ui/" in path_str and ("mainwindow" in path_str.lower() or "dialog" in path_str.lower())
        is_process_controller = "process" in path_str.lower() or "controller" in path_str.lower()
        is_proxy_setter = "proxy" in path_str.lower() and "set" in path_str.lower()
        is_log_model = "log" in path_str.lower() and "model" in path_str.lower()
        
        if is_ui_implementation or is_process_controller or is_proxy_setter or is_log_model:
            category = "A类 (接近上限)"
            reason = f"属于A类文件，当前{lines}行，接近600行上限"
            suggestion = "建议保持在450-600行范围内，超过600必须拆分"
        else:
            category = "C类 (建议拆分)"
            reason = f"文件{lines}行，≥450行建议拆分"
            suggestion = "建议按职责拆分，避免多主题混杂"
    elif lines >= 300:
        if "mainwindow" in path_str.lower():
            category = "B类 (MainWindow)"
            reason = f"MainWindow相关文件，当前{lines}行"
            suggestion = "建议控制在300-450行，超过说明UI在做流程编排"
        else:
            category = "B类 (建议优化)"
            reason = f"文件{lines}行，在300-450范围内"
            suggestion = "建议保持在300-450行范围内"
    else:
        category = "正常"
        reason = f"文件{lines}行，符合规范"
        suggestion = "无需优化"
    
    return FileAnalysis(
        path=rel_path,
        lines=lines,
        category=category,
        reason=reason,
        suggestion=suggestion
    )

def main():
    """主函数"""
    project_root = Path.cwd()
    
    # 需要分析的目录（排除3rdparty）
    analyze_dirs = [
        "ui", "main", "db", "fmt", "sys", "sub", "rpc"
    ]
    
    all_files = []
    
    # 收集所有需要分析的文件
    for dir_name in analyze_dirs:
        dir_path = project_root / dir_name
        if dir_path.exists():
            for ext in ['*.cpp', '*.hpp', '*.h']:
                for file_path in dir_path.rglob(ext):
                    all_files.append(file_path)
    
    # 分析每个文件
    analyses = []
    for file_path in sorted(all_files):
        lines = count_code_lines(file_path)
        analysis = analyze_file(file_path, lines)
        analyses.append(analysis)
    
    # 生成报告
    report_lines = []
    report_lines.append("# 文件大小优化报告")
    report_lines.append("")
    report_lines.append("本报告基于 file-size-limit 规范生成，用于识别需要优化的文件。")
    report_lines.append("")
    report_lines.append("## 规范说明")
    report_lines.append("")
    report_lines.append("- **A类**：允许接近600行（450-600 OK），但超过600必须拆分")
    report_lines.append("  - 单窗口/单页面 UI 实现")
    report_lines.append("  - 核心进程控制器")
    report_lines.append("  - 单平台系统代理设置器")
    report_lines.append("  - 日志 Model / 解析适配器")
    report_lines.append("")
    report_lines.append("- **B类**：建议300-450行")
    report_lines.append("  - MainWindow.cpp / AppController.cpp")
    report_lines.append("  - ConfigRepository.cpp")
    report_lines.append("  - SystemIntegration 相关")
    report_lines.append("")
    report_lines.append("- **C类**：必须拆分（≥450基本就拆）")
    report_lines.append("  - 多主题混杂、巨型分发逻辑、跨层依赖、函数过长")
    report_lines.append("")
    report_lines.append("---")
    report_lines.append("")
    
    # 按类别分组
    must_split = [a for a in analyses if "必须拆分" in a.category]
    category_a = [a for a in analyses if "A类" in a.category]
    category_b = [a for a in analyses if "B类" in a.category]
    category_c = [a for a in analyses if "C类" in a.category]
    normal = [a for a in analyses if a.category == "正常"]
    generated = [a for a in analyses if a.category == "GENERATED"]
    
    # 必须拆分的文件
    if must_split:
        report_lines.append("## 🔴 必须拆分（超过600行）")
        report_lines.append("")
        for a in sorted(must_split, key=lambda x: x.lines, reverse=True):
            report_lines.append(f"### {a.path}")
            report_lines.append(f"- **行数**: {a.lines}")
            report_lines.append(f"- **原因**: {a.reason}")
            report_lines.append(f"- **建议**: {a.suggestion}")
            report_lines.append("")
    
    # C类文件
    if category_c:
        report_lines.append("## 🟠 C类：建议拆分（≥450行）")
        report_lines.append("")
        for a in sorted(category_c, key=lambda x: x.lines, reverse=True):
            report_lines.append(f"### {a.path}")
            report_lines.append(f"- **行数**: {a.lines}")
            report_lines.append(f"- **原因**: {a.reason}")
            report_lines.append(f"- **建议**: {a.suggestion}")
            report_lines.append("")
    
    # A类文件
    if category_a:
        report_lines.append("## 🟡 A类：接近上限（450-600行）")
        report_lines.append("")
        for a in sorted(category_a, key=lambda x: x.lines, reverse=True):
            report_lines.append(f"### {a.path}")
            report_lines.append(f"- **行数**: {a.lines}")
            report_lines.append(f"- **原因**: {a.reason}")
            report_lines.append(f"- **建议**: {a.suggestion}")
            report_lines.append("")
    
    # B类文件
    if category_b:
        report_lines.append("## 🟢 B类：建议优化（300-450行）")
        report_lines.append("")
        for a in sorted(category_b, key=lambda x: x.lines, reverse=True):
            report_lines.append(f"### {a.path}")
            report_lines.append(f"- **行数**: {a.lines}")
            report_lines.append(f"- **原因**: {a.reason}")
            report_lines.append(f"- **建议**: {a.suggestion}")
            report_lines.append("")
    
    # 统计信息
    report_lines.append("---")
    report_lines.append("")
    report_lines.append("## 统计信息")
    report_lines.append("")
    report_lines.append(f"- 必须拆分（>600行）: {len(must_split)} 个文件")
    report_lines.append(f"- C类（≥450行）: {len(category_c)} 个文件")
    report_lines.append(f"- A类（450-600行）: {len(category_a)} 个文件")
    report_lines.append(f"- B类（300-450行）: {len(category_b)} 个文件")
    report_lines.append(f"- 正常（<300行）: {len(normal)} 个文件")
    report_lines.append(f"- 自动生成: {len(generated)} 个文件")
    report_lines.append(f"- **总计**: {len(analyses)} 个文件")
    report_lines.append("")
    
    # 所有文件列表（按行数排序）
    report_lines.append("---")
    report_lines.append("")
    report_lines.append("## 所有文件列表（按行数降序）")
    report_lines.append("")
    report_lines.append("| 文件路径 | 行数 | 分类 | 状态 |")
    report_lines.append("|---------|------|------|------|")
    
    for a in sorted(analyses, key=lambda x: x.lines, reverse=True):
        if a.category == "GENERATED":
            status = "✅ 自动生成"
        elif a.lines > 600:
            status = "🔴 必须拆分"
        elif a.lines >= 450:
            status = "🟠 建议拆分"
        elif a.lines >= 300:
            status = "🟢 建议优化"
        else:
            status = "✅ 正常"
        
        report_lines.append(f"| {a.path} | {a.lines} | {a.category} | {status} |")
    
    # 写入报告文件
    report_path = project_root / "file_size_optimization_report.md"
    with open(report_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(report_lines))
    
    print(f"报告已生成: {report_path}")
    print(f"\n统计:")
    print(f"  必须拆分: {len(must_split)} 个")
    print(f"  C类（≥450）: {len(category_c)} 个")
    print(f"  A类（450-600）: {len(category_a)} 个")
    print(f"  B类（300-450）: {len(category_b)} 个")
    print(f"  正常: {len(normal)} 个")

if __name__ == "__main__":
    main()
