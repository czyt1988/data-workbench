#include "DAAppProjectActionPolicy.h"
#include "DAAppWindowStateSerializer.h"
#include <QDebug>

namespace
{
int fail(const char* msg)
{
    qCritical() << msg;
    return 1;
}
}  // namespace

int main()
{
    using namespace DA;

    if (resolveAppCloseAction(false, DAAppSavePromptChoice::Cancel) != DAAppCloseAction::CloseDirectly) {
        return fail("close action should ignore prompt choice when project is clean");
    }
    if (resolveAppCloseAction(true, DAAppSavePromptChoice::Save) != DAAppCloseAction::SaveThenClose) {
        return fail("dirty close with save should request save first");
    }
    if (resolveAppCloseAction(true, DAAppSavePromptChoice::Discard) != DAAppCloseAction::CloseDirectly) {
        return fail("dirty close with discard should close directly");
    }
    if (resolveAppCloseAction(true, DAAppSavePromptChoice::Cancel) != DAAppCloseAction::CancelClosing) {
        return fail("dirty close with cancel should abort closing");
    }

    if (resolveAppOpenPreparation(false, false, DAAppSavePromptChoice::Discard)
        != DAAppOpenPreparation::OpenDirectly) {
        return fail("clean empty project should open directly");
    }
    if (resolveAppOpenPreparation(true, false, DAAppSavePromptChoice::Discard)
        != DAAppOpenPreparation::ConfirmReplaceThenOpen) {
        return fail("clean existing project should confirm replacement");
    }
    if (resolveAppOpenPreparation(true, true, DAAppSavePromptChoice::Save)
        != DAAppOpenPreparation::SaveThenOpen) {
        return fail("dirty project with save should save before opening");
    }
    if (resolveAppOpenPreparation(true, true, DAAppSavePromptChoice::Discard)
        != DAAppOpenPreparation::OpenDirectly) {
        return fail("dirty project with discard should open directly");
    }
    if (resolveAppOpenPreparation(true, true, DAAppSavePromptChoice::Cancel)
        != DAAppOpenPreparation::CancelOpening) {
        return fail("dirty project with cancel should abort opening");
    }

    {
        DAAppWindowStateSnapshot state;
        state.geometry        = QByteArrayLiteral("geometry");
        state.mainWindowState = QByteArrayLiteral("state");
        state.dockingState    = QByteArrayLiteral("docking");

        const QByteArray bytes = serializeAppWindowState(state);
        DAAppWindowStateSnapshot decoded;
        if (!deserializeAppWindowState(bytes, &decoded)) {
            return fail("serialized window state should be readable");
        }
        if (decoded.geometry != state.geometry) {
            return fail("window geometry should survive serialization");
        }
        if (decoded.mainWindowState != state.mainWindowState) {
            return fail("main window state should survive serialization");
        }
        if (decoded.dockingState != state.dockingState) {
            return fail("docking state should survive serialization");
        }
    }

    {
        DAAppWindowStateSnapshot state;
        state.geometry        = QByteArrayLiteral("geometry");
        state.mainWindowState = QByteArrayLiteral("state");

        const QByteArray bytes = serializeAppWindowState(state);
        DAAppWindowStateSnapshot decoded;
        if (!deserializeAppWindowState(bytes, &decoded)) {
            return fail("window state without docking payload should still decode");
        }
        if (!decoded.dockingState.isEmpty()) {
            return fail("missing docking payload should decode as empty");
        }
    }

    {
        DAAppWindowStateSnapshot decoded;
        if (deserializeAppWindowState(QByteArrayLiteral("broken-payload"), &decoded)) {
            return fail("broken payload should not decode successfully");
        }
    }

    return 0;
}
