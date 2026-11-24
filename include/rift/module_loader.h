#pragma once

#include "ast.h"

#include <filesystem>

namespace rift {

Program loadProgram(const std::filesystem::path& entryPath);

}
