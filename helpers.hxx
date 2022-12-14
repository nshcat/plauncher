#pragma once

#include <Windows.h>

#define RETURNHRSILENT_IF_ERROR(hr) do { if(FAILED(hr)) return hr; } while(0);