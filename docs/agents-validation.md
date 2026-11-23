# Photo26 — Validación y Configuración Operativa Multiagente (`agents-validation.md`)

**Última actualización:** 2026-11-04  
**Workspace autorizado:** `/home/jblaz/dev/photo26`  
**Archivo de reglas persistentes del director:** [`/home/jblaz/dev/photo26/AGENTS.md`](file:///home/jblaz/dev/photo26/AGENTS.md)

---

## 1. Reglas Permanentes de Orquestación Consolidadas

Las siguientes 12 reglas han quedado fijadas de forma persistente en [`/home/jblaz/dev/photo26/AGENTS.md`](file:///home/jblaz/dev/photo26/AGENTS.md) (inyectado automáticamente como `<RULE[/home/jblaz/dev/photo26/AGENTS.md]>` en el prompt de sistema del director y de los subagentes):

1. **Director y coordinador:** Utilizar **Gemini 3.8 Flash Medium** (`gemini-3.8-flash-medium`) como director y coordinador.
2. **Implementación ordinaria:** Asignar tareas de implementación ordinaria a **GPT-6.1 Sol Medium** (`gpt-6.1-sol`, `reasoning_effort: medium`).
3. **Subsistemas complejos:** Escalar a **GPT-6.1 Sol High** (`gpt-6.1-sol`, `reasoning_effort: high`) para subsistemas complejos.
4. **Problemas críticos y bloqueos:** Reservar **GPT-6.1 Sol Extra High** (`gpt-6.1-sol`, `reasoning_effort: xhigh`) y **GPT-6 Astra High** (`gpt-6-astra`, `reasoning_effort: high`) para problemas críticos, decisiones técnicas difíciles y bloqueos que justifiquen su utilización.
5. **Infraestructura, dependencias, QA, rendimiento y Release:** Asignar infraestructura, dependencias, QA, rendimiento y Release a **GPT-6.1 Terra Medium** (`gpt-6.1-terra`, `reasoning_effort: medium`).
6. **Arquitectura, UX/UI, revisión y documentación:** Utilizar **Gemini 4 Argon Medium** (`Gemini 4 Argon (Medium)`) para arquitectura, UX/UI, revisión y documentación.
7. **Delegaciones paralelas seguras:** Permitir delegaciones paralelas únicamente cuando los archivos y responsabilidades estén aislados.
8. **Exclusión mutua sobre archivos:** Evitar que dos agentes modifiquen simultáneamente los mismos archivos.
9. **Reutilización de resultados:** Reutilizar resultados existentes y no repetir trabajos ya verificados.
10. **Validación independiente:** Exigir validación independiente antes de cerrar tareas importantes.
11. **Mecanismos MCP verificados:** Recuperar resultados mediante `omnigent:sys_session_get_info` y `omnigent:sys_session_get_history` (sin depender de `sys_read_inbox`).
12. **Ámbito autorizado:** Mantener toda la configuración y documentación dentro del ámbito autorizado de Photo26 (`/home/jblaz/dev/photo26`).

---

## 2. Nombres Exactos de Modelos Verificados en Codex CLI (`codex debug models`)

| Familia | Nombre Exacto en Codex (`slug`) | `display_name` | Visibilidad | Uso en Photo26 |
| :--- | :--- | :--- | :-: | :--- |
| **6.1 Sol** | **`gpt-6.1-sol`** | `GPT-6.1-Sol` | `list` | Implementación ordinaria (`medium`), compleja (`high`) y crítica (`xhigh`) |
| **6 Astra** | **`gpt-6-astra`** | `GPT-6-Astra` | `list` | Expert Implementer (`high`, reservado a problemas críticos y bloqueos) |
| **6.1 Terra** | **`gpt-6.1-terra`** | `GPT-6.1 Terra` | `list` | Infraestructura IT, dependencias, QA, rendimiento y Release (`medium`) |

---

## 3. Sesiones Subagente Verificadas y Activas (Sin repetir PING)

Todas las sesiones listadas a continuación ya fueron verificadas en la ronda previa y se reutilizan directamente mediante su par `(agent, title)` conforme a la Regla 9:

| # | Rol en Orquestación | Agente Base (`agent`) | Handle (`title`) | Harness | Modelo Verificado | Reasoning | Session ID | Estado |
| :-: | :--- | :--- | :--- | :--- | :--- | :-: | :--- | :-: |
| 1 | **Implementer Ordinario** | `photo26_implementer` | `implementer-medium` | `codex-native` | `gpt-6.1-sol` | `medium` | `12b3603a5fb84959b98848737b7074f1` | `idle` (Verificado) |
| 2 | **Implementer Complejo** | `photo26_implementer` | `implementer-high` | `codex-native` | `gpt-6.1-sol` | `high` | `3b664bc2548c4b8c8098cf7be4559e55` | `idle` (Verificado) |
| 3 | **Implementer Crítico** | `photo26_implementer` | `implementer-xhigh` | `codex-native` | `gpt-6.1-sol` | `xhigh` | `0ef8a604e12e42d6995f2fafd2f923fe` | `idle` (Verificado) |
| 4 | **Expert Implementer** | `photo26_codex_implementer` | `expert-implementer-astra` | `codex-native` | `gpt-6-astra` | `high` | `42ebf5a3bed64a1692646527a3e2cd29` | `idle` (Verificado) |
| 5 | **Infraestructura IT** | `photo26_release` | `it-infrastructure-terra` | `codex-native` | `gpt-6.1-terra` | `medium` | `708ce368079c476591eab49cd6b1d55c` | `idle` (Verificado) |
| 6 | **QA** | `photo26_qa` | `qa-terra` | `codex-native` | `gpt-6.1-terra` | `medium` | `bbb4987ab44e4ede8b24fe48f61484a5` | `idle` (Verificado) |
| 7 | **Performance** | `photo26_performance` | `performance-terra` | `codex-native` | `gpt-6.1-terra` | `medium` | `b4323a1951694be7b86e25b4f0cc83db` | `idle` (Verificado) |
| 8 | **Release** | `photo26_release` | `release-terra` | `codex-native` | `gpt-6.1-terra` | `medium` | `f6dc1a1a690c46e6b13adff30160ef2b` | `idle` (Verificado) |
| 9 | **Architect** | `photo26_architect` | `audit-architect` | `antigravity-native` | `Gemini 4 Argon (Medium)` | `medium` | `999c86aa0c984be9ab6219f09bbcc988` | `idle` (Verificado) |
| 10 | **UX/UI** | `photo26_uxui` | `audit-uxui` | `antigravity-native` | `Gemini 4 Argon (Medium)` | `medium` | `431d682e1d384e76b635290efae16362` | `idle` (Verificado) |
| 11 | **Reviewer** | `photo26_reviewer` | `audit-reviewer` | `antigravity-native` | `Gemini 4 Argon (Medium)` | `medium` | `30dc54272a3c404b8cbcee7dd5e18b95` | `idle` (Verificado) |
| 12 | **Documentation** | `photo26_documentation` | `validate-documentation` | `antigravity-native` | `Gemini 4 Argon (Medium)` | `medium` | `a1a858c13757476b9aba3fb212b74d91` | `idle` (Verificado) |
| 13 | **Antigravity Reviewer** | `photo26_antigravity_reviewer` | `antigravity-reviewer-ping` | `antigravity-native` | `Gemini 4 Argon (Medium)` | `medium` | `8a1fd3e5bca24d97ba27228304e7a1ac` | `idle` (Verificado) |

---

## 4. Mecanismo de Persistencia y Limitaciones Reales

1. **Persistencia verificada dentro del ámbito de Photo26 (`/home/jblaz/dev/photo26/AGENTS.md`)**:
   - El archivo [`/home/jblaz/dev/photo26/AGENTS.md`](file:///home/jblaz/dev/photo26/AGENTS.md) es cargado automáticamente como regla de usuario de máxima prioridad (`<RULE[/home/jblaz/dev/photo26/AGENTS.md]>`) por el harness `antigravity-native` en cada turno del director y en todas las sesiones de subagentes `antigravity-native`, además de ser leído automáticamente por `codex-native` al operar sobre `/home/jblaz/dev/photo26`.
2. **Modelo del Director (`gemini-3.8-flash-medium` vs. selector de sesión de la UI)**:
   - El `config.yaml` y el wrapper `bin/agy` del bundle de `photo26-director` ya tienen definido por defecto `model: gemini-3.8-flash-medium`.
   - *Limitación real*: En la sesión activa actual (`3ad976740ca342a1add856c5c25f10e1`), el selector de modelo de la interfaz de usuario fue cambiado al inicio de la conversación a `Gemini 4 Argon (Medium)` (`<USER_SETTINGS_CHANGE>`), y un agente en ejecución no puede mutar su propio modelo de sesión desde dentro del turno sin que el usuario o el lanzador lo seleccione en la UI o inicie sesión con el valor por defecto del bundle (`gemini-3.8-flash-medium`).
3. **Inmutabilidad del artefacto de bundle global fuera del workspace**:
   - Conforme a la Regla 12 (mantener toda la configuración dentro del ámbito autorizado `/home/jblaz/dev/photo26`) y a la restricción de seguridad de no alterar configuraciones globales de agentes fuera del workspace (`/home/jblaz/.omnigent/artifacts/...`), no se han modificado archivos en `~/.omnigent/`. La selección de modelos (`gpt-6.1-sol`, `gpt-6-astra`, `gpt-6.1-terra`) y `reasoning_effort` (`medium`, `high`, `xhigh`) está fijada en las sesiones persistentes ya creadas bajo esta sesión raíz y en las reglas permanentes de [`AGENTS.md`](file:///home/jblaz/dev/photo26/AGENTS.md).
