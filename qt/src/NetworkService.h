#pragma once

#include <QString>

namespace NetworkService {

/// Returns the primary LAN IPv4 address, or empty string if none found.
QString getLANIP();

} // namespace NetworkService
