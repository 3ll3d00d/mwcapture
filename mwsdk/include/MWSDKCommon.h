

#pragma once


#define DECLARE_MWHANDLE64(name) struct name##__{int unused;}; typedef struct name##__ * __ptr64 name
