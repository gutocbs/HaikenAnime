# Lifecycle and Shutdown Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make synchronization shutdown explicit, idempotent, ordered before logger shutdown, and testable while preserving the fixture-backed execution path.

**Architecture:** `InitialSyncCoordinator` remains the scheduler and owns each short-lived `QThread`, but gains an explicit terminal shutdown state and an injectable synchronization operation. Production builds that operation from the current fixture/SQLite flow inside the worker thread; tests inject controlled operations while exercising the real coordinator and real `QThread`.

**Tech Stack:** C++20, Qt 6 Core, Qt Test, CMake/CTest, SQLite adapter, Markdown.

**Spec:** `docs/superpowers/specs/2026-09-24-lifecycle-shutdown-design.md`

## Global Constraints

- Keep `FileAniListDataSource` and the local fixture as the active synchronization provider.
- Do not add a partial cancellation flag or claim that waiting for a thread cancels its operation.
- Create SQLite objects and repositories inside the worker thread that uses them.
- Stop synchronization before stopping `AsyncLogger`.
- Preserve the existing synchronization signals and visible frontend behavior.
- Use English for C++ identifiers and test names; keep the pedagogical documentation in Portuguese.
- Write each behavior test first, observe the expected failure, then implement the minimum production change.
- Do not alter the real GraphQL connection, retry, OAuth, rate-limit, or mutation behavior in this item.

## Review Focus

- `start()` called twice while the first operation is blocked must execute only one operation; covered in Task 2.
- `shutdown()` called before any `start()` must be safe and permanently reject later starts; covered in Task 2.
- `shutdown()` called during an active operation must not return before that operation exits; covered in Task 2.
- A completion callback queued while shutdown blocks the main thread must not restart the scheduler afterward; covered in Task 2.
- Repeated `shutdown()` and destructor execution must not wait on or delete the same `QThread` twice; covered in Task 2.

---

### Task 1: Record deferred AniList integration work

**Files:**
- Modify: `planning.md`

**Interfaces:**
- Consumes: item 6 findings already documented in `review.md`.
- Produces: durable planning entries for naming, synchronous transport, grouped mutation atomicity, and the item 6 debt reference.

- [ ] **Step 1: Add mapper naming debt under `Integração com o AniList`**

Add this planning entry:

```markdown
- Normalizar a nomenclatura das funções de leitura do `AniListMediaMapper`. Fixture e GraphQL possuem formatos diferentes e justificam funções separadas, enquanto o mapeamento comum para domínio evita duplicação; a mudança de estilo não deve ser apresentada como validação da integração real.
```

- [ ] **Step 2: Consolidate the synchronous transport debt**

Replace or expand the existing retry/event-loop bullets so the section contains one cohesive entry:

```markdown
- Substituir o transporte síncrono atual em conjunto com a revisão de concorrência, scheduler, rate limit e cancelamento ponta a ponta. `QEventLoop` aninhado permite reentrância, `QThread::msleep` bloqueia a thread chamadora e retry recursivo mistura resiliência com transporte; os testes por fixture não exercitam esses custos.
```

- [ ] **Step 3: Make grouped mutation limitations explicit**

Expand the existing `SaveMediaListEntry` entry with:

```markdown
O agrupamento atual prepara o contrato, mas não garante atomicidade: mutations separadas podem aplicar apenas parte dos campos. A consolidação deverá ser feita quando o contrato remoto for validado, junto com idempotência e reconciliação da outbox.
```

- [ ] **Step 4: Extend the central debt reference**

Update `Dívidas identificadas durante a revisão do core` so its single reference explicitly includes integração AniList. Do not duplicate the entire list from `review.md`.

- [ ] **Step 5: Validate the documentation diff**

Run:

```powershell
git diff --check -- planning.md
```

Expected: exit code 0 with no whitespace errors.

- [ ] **Step 6: Commit the planning update**

```powershell
git add planning.md
git commit -m "docs: registrar dividas da integracao AniList"
```

### Task 2: Make coordinator shutdown explicit and testable

**Files:**
- Create: `tests/unit/InitialSyncCoordinatorTests.cpp`
- Modify: `src/app/InitialSyncCoordinator.h`
- Modify: `src/app/InitialSyncCoordinator.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `QThread`, `QTimer`, `QString &error`, and the existing `started`, `completed`, and `failed` signals.
- Produces: `using SyncOperation = std::function<bool(QString &)>`, the test seam constructor `InitialSyncCoordinator(SyncOperation, int, QObject *)`, and public slot `void shutdown()`.

- [ ] **Step 1: Read the project test rules before writing tests**

Read completely:

```text
C:/Users/gutoc/.codex/plugins/cache/openai-curated-remote/superpowers/6.4.1/skills/test-driven-development/writing-good-tests.md
```

The tests must assert coordinator behavior and use a real `QThread`; do not assert calls on a fake thread.

- [ ] **Step 2: Add the test target with a deliberately missing lifecycle API**

Add `InitialSyncCoordinatorTests` to `CMakeLists.txt` with:

```cmake
add_executable(InitialSyncCoordinatorTests
        tests/unit/InitialSyncCoordinatorTests.cpp
        src/app/InitialSyncCoordinator.h
        src/app/InitialSyncCoordinator.cpp
        src/infrastructure/logging/AsyncLogger.h
        src/infrastructure/logging/AsyncLogger.cpp
        src/infrastructure/database/SqliteDatabase.h
        src/infrastructure/database/SqliteDatabase.cpp
        src/infrastructure/database/SqliteMediaMapper.h
        src/infrastructure/database/SqliteMediaMapper.cpp
        src/infrastructure/database/SqliteMediaRepository.h
        src/infrastructure/database/SqliteMediaRepository.cpp
        src/infrastructure/database/SqlQueryStore.h
        src/infrastructure/database/SqlQueryStore.cpp
        src/infrastructure/anilist/FileAniListDataSource.h
        src/infrastructure/anilist/FileAniListDataSource.cpp
        src/infrastructure/anilist/AniListMediaMapper.h
        src/infrastructure/anilist/AniListMediaMapper.cpp
        src/application/anilist/AniListSyncService.h
        src/application/anilist/AniListSyncService.cpp
)

target_link_libraries(InitialSyncCoordinatorTests PRIVATE Qt6::Core Qt6::Sql Qt6::Test)
add_test(NAME InitialSyncCoordinatorTests COMMAND InitialSyncCoordinatorTests)
```

Also append `InitialSyncCoordinatorTests` to `HAIKENANIME_TESTS` so it receives the Qt runtime `PATH`.

- [ ] **Step 3: Write the first failing test for duplicate starts**

Create `tests/unit/InitialSyncCoordinatorTests.cpp` with a fixture containing atomic counters and `QSemaphore`s. The first test must have this observable contract:

```cpp
void InitialSyncCoordinatorTests::doesNotRunConcurrentSynchronizations() {
    std::atomic_int calls = 0;
    QSemaphore entered;
    QSemaphore release;
    InitialSyncCoordinator coordinator(
        [&](QString &) {
            ++calls;
            entered.release();
            release.acquire();
            return true;
        },
        60000);

    coordinator.start();
    QVERIFY(entered.tryAcquire(1, 1000));
    coordinator.start();
    QCOMPARE(calls.load(), 1);

    release.release();
    coordinator.shutdown();
}
```

Include `Q_OBJECT`, `QTEST_MAIN`, `<atomic>`, `QSemaphore`, `QSignalSpy`, `QThread`, and the coordinator header. Give each later lifecycle behavior its own private slot.

- [ ] **Step 4: Run the test target and verify RED**

Run:

```powershell
cmake -S . -B cmake-build-validation -G Ninja -DBUILD_TESTING=ON
cmake --build cmake-build-validation --target InitialSyncCoordinatorTests
```

Expected: compilation fails because the operation constructor and `shutdown()` do not exist. This proves the test requests a new public behavior rather than passing against the old implementation.

- [ ] **Step 5: Add the minimal operation seam and shutdown state**

In `InitialSyncCoordinator.h`, add:

```cpp
#include <functional>

using SyncOperation = std::function<bool(QString &)>;

explicit InitialSyncCoordinator(SyncOperation operation,
                                int syncIntervalMs = 3600000,
                                QObject *parent = nullptr);

public slots:
    void start();
    void shutdown();

private:
    bool performSynchronization(QString &error) const;
    void releaseFinishedThread();
    SyncOperation operation_;
    bool stopping_ = false;
```

Keep the existing production constructor. Its worker lambda must choose `operation_` when injected and otherwise call `performSynchronization(error)`. Move the current fixture/SQLite body, unchanged in behavior, into `performSynchronization` and execute it only from the worker thread.

Implement the lifecycle core as:

```cpp
void InitialSyncCoordinator::start() {
    if (stopping_ || (thread_ && thread_->isRunning())) {
        return;
    }
    releaseFinishedThread();
    // Existing logging and started signal, then QThread::create(...).
}

void InitialSyncCoordinator::shutdown() {
    if (!stopping_) {
        stopping_ = true;
        scheduler_->stop();
    }
    if (thread_ && thread_->isRunning()) {
        thread_->wait();
    }
    releaseFinishedThread();
}

void InitialSyncCoordinator::releaseFinishedThread() {
    if (thread_ && !thread_->isRunning()) {
        delete thread_;
        thread_ = nullptr;
    }
}
```

The destructor must call `shutdown()`. The queued completion lambda must start `scheduler_` only when `!stopping_`.

- [ ] **Step 6: Run the duplicate-start test and verify GREEN**

Run:

```powershell
cmake --build cmake-build-validation --target InitialSyncCoordinatorTests
ctest --test-dir cmake-build-validation -R InitialSyncCoordinatorTests --output-on-failure
```

Expected: `InitialSyncCoordinatorTests` passes.

- [ ] **Step 7: Add a failing test that shutdown waits for active work**

Add a test whose operation signals `entered`, waits on `release`, then sets `finished = true`. Start an independent `std::thread` that waits until the operation entered, sleeps briefly, and releases it. Call `coordinator.shutdown()` on the test/owner thread and assert `finished` is true immediately after shutdown returns:

```cpp
std::atomic_bool finished = false;
QSemaphore entered;
QSemaphore release;
InitialSyncCoordinator coordinator([&](QString &) {
    entered.release();
    release.acquire();
    finished = true;
    return true;
}, 60000);
coordinator.start();
QVERIFY(entered.tryAcquire(1, 1000));

std::thread releaser([&] {
    QThread::msleep(20);
    release.release();
});
coordinator.shutdown();
releaser.join();
QVERIFY(finished.load());
```

Temporarily remove the wait from `shutdown()` only to perform the mutation check if necessary; the test must fail or become observably unsafe without the wait, then restore the implementation.

- [ ] **Step 8: Add tests for terminal and idempotent shutdown**

Add separate tests:

```cpp
void shutdownBeforeStartRejectsFutureStart();
void repeatedShutdownIsSafe();
void completionDuringShutdownDoesNotRestartScheduler();
```

For `shutdownBeforeStartRejectsFutureStart`, call shutdown, then start, process events with `QTest::qWait(30)`, and compare `calls == 0`.

For `repeatedShutdownIsSafe`, complete one operation, call shutdown twice, and assert `calls == 1` without a crash or hang.

For `completionDuringShutdownDoesNotRestartScheduler`, use a 10 ms scheduler interval, hold the first operation with a semaphore, release it from an independent thread while `shutdown()` waits, process events for at least 40 ms after shutdown, and assert `calls == 1`.

- [ ] **Step 9: Run all coordinator tests**

Run:

```powershell
cmake --build cmake-build-validation --target InitialSyncCoordinatorTests
ctest --test-dir cmake-build-validation -R InitialSyncCoordinatorTests --output-on-failure
```

Expected: all coordinator lifecycle cases pass with no timeout, crash, or `QThread: Destroyed while thread is still running` diagnostic.

- [ ] **Step 10: Commit the coordinator lifecycle**

```powershell
git add CMakeLists.txt src/app/InitialSyncCoordinator.h src/app/InitialSyncCoordinator.cpp tests/unit/InitialSyncCoordinatorTests.cpp
git commit -m "fix: explicitar ciclo de vida da sincronizacao"
```

### Task 3: Enforce application shutdown order

**Files:**
- Modify: `main.cpp`

**Interfaces:**
- Consumes: `InitialSyncCoordinator::shutdown()` from Task 2 and `AsyncLogger::stop()`.
- Produces: application-level order `initialSync shutdown -> logger stop`.

- [ ] **Step 1: Update the `aboutToQuit` handler**

Replace the handler body with:

```cpp
QObject::connect(&app, &QCoreApplication::aboutToQuit, [&context]() {
    if (context.initialSync) {
        context.initialSync->shutdown();
    }
    if (context.logger) {
        context.logger->stop();
    }
});
```

Do not reverse these calls: the synchronization owns no logger, but still holds a non-owning pointer and may log while finishing.

- [ ] **Step 2: Build the complete application target**

Run:

```powershell
cmake --build cmake-build-validation --target HaikenAnime
```

Expected: exit code 0 and `HaikenAnime.exe` linked successfully.

- [ ] **Step 3: Run coordinator tests again**

Run:

```powershell
ctest --test-dir cmake-build-validation -R InitialSyncCoordinatorTests --output-on-failure
```

Expected: all coordinator lifecycle tests pass.

- [ ] **Step 4: Commit the shutdown order**

```powershell
git add main.cpp
git commit -m "fix: encerrar sincronizacao antes do logger"
```

### Task 4: Document item 7 pedagogically

**Files:**
- Modify: `review.md`
- Modify: `planning.md`

**Interfaces:**
- Consumes: implemented lifecycle behavior from Tasks 2 and 3 and existing debt-reference convention.
- Produces: completed item 7 review plus non-duplicated planning references.

- [ ] **Step 1: Mark item 7 reviewed**

Change the status line near the top of `review.md` to:

```markdown
7. Composição, concorrência e ciclo de vida — revisados.
```

- [ ] **Step 2: Add the item 7 teaching section**

Append `## 7. Composição, concorrência e ciclo de vida` after item 6, with these subsections and exact conclusions:

```markdown
### Escopo desta parte
### Composition root e ownership
### Afinidade de thread e SQLite
### Coordenador, scheduler e exclusão mútua
### Shutdown explícito
### Esperar não é cancelar
### Ciclo de vida do logger
### Workers existentes, mas não compostos
### Dívidas identificadas, mas não corrigidas nesta parte
### Resultado da parte 7
### Validação executada
```

Explain, rather than merely list, the following:

- C++ destroys `ApplicationContext` members in reverse declaration order, so `initialSync` dies before repositories, database, and logger.
- The background database and repository are constructed inside their owning worker thread; copying a `QSqlDatabase` handle does not make it cross-thread safe.
- `start()` provides in-process mutual exclusion, not a persistent distributed scheduler lock.
- `shutdown()` stops future scheduling and waits, but cannot interrupt fixture/SQLite/HTTP calls.
- `AsyncLogger::stop()` drains its condition-variable loop; `QThread::quit()` alone does not terminate `run()` because the direct-connected function occupies that thread.
- `AniListSyncWorker` and `AniListUpdateWorker` are not used by the current composition and hold non-owning service references.
- The injection seam separates scheduling from work and enables real-thread tests; it is not a second production provider.

The debt list must include cancellation, potentially unbounded wait, persistent scheduler state, logger failure visibility, worker ownership, and the synchronous GraphQL transport. Do not claim real AniList validation.

- [ ] **Step 3: Update `planning.md` without duplicating debt details**

Add under `Threading e ciclo de vida`:

```markdown
- Definir uma política para operações que não retornam durante shutdown. O coordenador agora impede novos trabalhos e aguarda a execução ativa, mas espera bloqueante não substitui cancelamento ponta a ponta nem oferece prazo máximo seguro para encerramento.
- Decidir se os workers futuros serão persistentes e movidos para threads com event loop. Essa evolução exige ownership explícito de worker, serviço, banco e transporte, além de destruição na thread correta.
```

Extend the central debt-reference sentence so it includes composição, concorrência e ciclo de vida. Keep detailed reasoning in `review.md`.

- [ ] **Step 4: Run the full verification**

Run:

```powershell
cmake --build cmake-build-validation
ctest --test-dir cmake-build-validation --output-on-failure
git diff --check
```

Expected: complete build exits 0, all CTest tests pass, and diff check exits 0.

- [ ] **Step 5: Request independent code review**

Use `superpowers:requesting-code-review` with a reviewer focused on:

- use-after-free or cross-thread access;
- deadlock and repeated-shutdown paths;
- queued callback behavior after shutdown;
- preservation of fixture execution;
- documentation claims matching tested behavior.

Fix every Critical or Important finding with a failing regression test before proceeding.

- [ ] **Step 6: Commit the review documentation**

```powershell
git add review.md planning.md docs/superpowers/specs/2026-09-24-lifecycle-shutdown-design.md docs/superpowers/plans/2026-09-24-lifecycle-shutdown.md
git commit -m "docs: revisar composicao concorrencia e ciclo de vida"
```
