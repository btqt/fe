---
description: "General rules for Python coding, environment execution, and mandatory English standards."
applyTo: "**/*.py"
---

# Python Coding & Execution Conventions

## Language Standards

- **English Only**: All code, including comments, docstrings, variable names, and error messages, MUST be written in English.

## Environment & Execution

- **Virtual Environment**: Use the environment located in the `.venv` directory at the project/workspace root.
- **Python Execution**: Always active the virtual environment before running scripts. Prefer executing via the `.venv` path to ensure dependency consistency.

## Environment Variables & Credentials

- **Mandatory .env**: Sensitive information (tokens, passwords, specific credentials) must never be hardcoded or stored in static config files. Use `python-dotenv` to load configurations from a `.env` file.

## Configuration Pattern

- **Config Injection**: When loading local configuration files, inject mandatory values from environment variables into the configuration dictionary to maintain security and flexibility.
- **Library**: Use `python-dotenv` for managing environment variables.
