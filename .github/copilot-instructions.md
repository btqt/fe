# Copilot Instructions

## 1. Source Code Management

- Whenever generating new code files or scripts, **ALWAYS** create and save them in the `tmp_script` directory to avoid cluttering the root directory.
- The `tmp_script` directory must be created automatically if it does not exist.
- Do not place temporary or experimental scripts in the project root.

---

## 2. Reports and Output Management

- Whenever generating report files, analysis documents, output files, or output from scripts, **ALWAYS** write/save them in the `tmp_output` directory.
- The `tmp_output` directory must be created automatically if it does not exist.
- Never store generated reports directly in the root directory.

---

## 3. Python Environment Management

- When running Python code or using Python-related tools:
  - **Prioritize using the local `.venv` virtual environment** in the current workspace.
  - On Windows, use:
    ```
    ./.venv/Scripts/python.exe
    ```
  - Avoid using the system-wide global `python` command unless absolutely necessary.

### UV Framework

- If the workspace uses the `uv` framework (detected via `uv.lock` or `pyproject.toml` with uv configuration):
  - **Prioritize using `uv run`** over other execution methods.
  - Follow uv’s dependency and environment management conventions.

---

## 4. Configuration File Requirement

- If the application requires configuration and no `config.yaml` exists, **ALWAYS create an appropriate `config.yaml` file**.
- The structure and content of `config.yaml` must comply with the rules defined in [`config-yaml.instructions.md`](./instructions/config-yaml.instructions.md).
- Never hardcode environment-dependent values directly inside source code when they belong in configuration.

---

## 5. Mermaid cho Flow và Diagram

- Khi cần thể hiện flow, diagram, hoặc workflow trong response hoặc file Markdown, **ưu tiên sử dụng Mermaid**.
- Chỉ dùng định dạng khác khi người dùng yêu cầu rõ ràng hoặc Mermaid không phù hợp với nội dung cần thể hiện.

---
