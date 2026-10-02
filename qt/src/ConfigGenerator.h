#pragma once

#include <QString>

struct GeneratedConfigs {
    QString nginxConfig;
};

class ConfigGenerator
{
public:
    GeneratedConfigs generate(const QString &hostIP);

private:
    QString nginxConfig() const;
    QString nginxUserDirective() const;
};
