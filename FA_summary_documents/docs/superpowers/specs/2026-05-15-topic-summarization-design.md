# Sequential Topic Analysis Design

## Goal
Process 40 architectural topics located in `raw_topics/`, reading their `doc.md` and `slide.md` one by one, to extract their core architectures and finally provide a summary/suggestion for structuring future architectural topics.

## Constraints & Requirements
- **Strict Sequential Order:** Topics must be processed exactly one by one.
- **File Reading Order:** For each topic, `doc.md` must be read first, followed by `slide.md`.
- **Language Requirements:** Summaries must be provided in Vietnamese, preserving technical terminology (e.g., UDS, ECU, DFMEA) in English.
- **Reporting Mechanism:** The assistant will pause and wait for the user's signal after summarizing each individual topic before moving to the next.
- **Task Tracking:** Progress must be recorded in `docs/plans/task.md`.

## Execution Flow (Per Topic)
1. Determine the next pending topic from `docs/plans/task.md`.
2. Read `raw_topics/[Topic Name]/doc.md`.
3. Read `raw_topics/[Topic Name]/slide.md`.
4. Output a brief summary via chat including:
   - **Problem:** The architectural challenge addressed.
   - **Solution:** The proposed design/patterns.
   - **Outcome:** Quality attributes or benefits.
5. Update the status of the topic in `docs/plans/task.md` to completed.
6. Await user confirmation ("OK", "Next", etc.) to proceed.

## Final Synthesis
After all 40 topics are completed, generate a comprehensive "Suggestion Outline" detailing how to structure new architecture topics based on common patterns found during the analysis.

## Scope
This design is strictly scoped to reading and summarizing the existing 40 topics in the `raw_topics` directory. No new topic creation will occur within this implementation phase; only the structure suggestion will be produced.
