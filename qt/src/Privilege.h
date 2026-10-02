#pragma once

#include <QString>

namespace Privilege {

/// True if the process can bind privileged ports (root / admin).
bool isElevated();

/// Re-launch this app with a graphical privilege prompt (pkexec / kdesu / runas).
/// Returns true if a new elevated process was started (caller should quit).
/// Returns false if the user cancelled or no elevating tool was found.
bool relaunchElevated(QString *errorMessage = nullptr);

} // namespace Privilege
