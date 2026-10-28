---
name: project-structure-hardening
description: "MUST USE when creating a new project or preparing an existing repository to use shared agent instructions and specs: AGENTS.md, CLAUDE.md, .github/copilot-instructions.md, .agents/skills, agent-os/standards, openspec, or packages/*/AGENTS.md. Use when the structure is missing, partial, inconsistent, or needs an idempotent audit. Do not use for ordinary application coding or for overwriting existing project rules."
---

# 固化项目目录结构

## 目标

为已有项目或新项目补齐统一的 Agent 工作目录。已有目录和文件默认保留，只有缺失项才创建；发现路径类型冲突或现有内容与目标要求冲突时停止并报告。

## 触发判断

以下任一情况触发本 Skill：

- 新项目需要建立 Agent 规则、Skill、规范和变更提案目录。
- 旧项目已有部分 `AGENTS.md`、`openspec`、`agent-os` 或 `.agents/skills`，需要补齐。
- 用户要求固化、规范化、初始化或审计项目 Agent 目录结构。

不要用于普通业务开发、复制项目规则、覆盖已有 `AGENTS.md`，或自动安装 OpenSpec、agent-os 等外部工具。

## 固化目标

```text
项目/
├── AGENTS.md
├── CLAUDE.md
├── .github/copilot-instructions.md
├── .agents/skills/<name>/SKILL.md
├── agent-os/standards/
├── openspec/changes/<id>/
├── openspec/specs/
└── packages/*/AGENTS.md
```

`AGENTS.md` 是唯一真相源。`CLAUDE.md` 和 `.github/copilot-instructions.md` 只提供适配引用，不复制规则内容。

## 执行方式

1. 确认目标项目根目录；优先使用用户明确给出的根目录，不能凭空改写其他目录。
2. 先执行检查模式，盘点目标路径的存在性、类型和已有内容。
3. 预检阶段发现文件/目录类型冲突时立即停止，不进行部分写入。
4. 检查通过后使用脚本的 `--apply` 执行幂等补齐。默认不覆盖已有文件、不删除内容、不创建虚构的 package。
5. 只处理 `packages/` 的直接子目录；空的 `packages/` 不生成虚构包。
6. 重新执行检查模式，逐项报告 `existing`、`created`、`skipped` 或 `conflict`。

## 必须使用的工具

```powershell
python scripts/solidify_project_structure.py --project-root <项目根目录> --skill-name <name>
python scripts/solidify_project_structure.py --project-root <项目根目录> --skill-name <name> --apply
```

脚本默认只检查。`--apply` 只创建缺失目录和缺失的适配文件；它不会覆盖已有文件。若目标项目不允许创建根级规则文件，应先检查并停止，不要用参数绕过冲突。

## 安全规则

- 不使用递归删除、全量重定向、无条件覆盖或危险通配符。
- 不把普通文件当作目录，也不把 `packages` 下的普通文件当作包。
- 不把用户未提供的 Skill 名称写成字面量 `<name>`。
- 已有内容与目标结构冲突时报告冲突，保留原文件，等待用户决定。
- 任何命令、脚本或审计失败后，先读取 `references/fallback.md`，再决定是否能安全修复。

## 支持文件

- 具体文件模板和幂等规则：`references/workflow.md`
- 失败恢复协议：`references/fallback.md`
- 触发正例与反例：`examples/positive.md`、`examples/negative.md`

