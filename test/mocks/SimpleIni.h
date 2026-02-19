#pragma once

#include <cstddef>
#include <string>

using SI_Error = int;

class CSimpleIniA {
public:
    void SetUnicode(bool = true) {}
    const char* GetValue(const char*, const char*, const char* def = "") const { return def; }
    SI_Error SetValue(const char*, const char*, const char*) { return 0; }
    SI_Error SetBoolValue(const char*, const char*, bool) { return 0; }
    SI_Error LoadData(const char*) { return 0; }
    void Save(std::string&) const {}
};
