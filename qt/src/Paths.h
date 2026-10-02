#pragma once

#include <QString>

namespace Paths {

QString workDir();
QString nginxConf();
QString nginxPid();
QString nginxErrLog();
QString dnsmasqConf(); // kept for optional external dnsmasq configs

void createWorkDir();

} // namespace Paths
