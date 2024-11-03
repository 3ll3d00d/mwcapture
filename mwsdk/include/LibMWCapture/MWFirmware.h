#pragma once

#include "KsMWCapFilter.h"

BOOL BackupFirmware(
	CKsMWCAPFilter * pFilter,
	LPCSTR pszMWFPath
	);

BOOL UpgradeFirmware(
	CKsMWCAPFilter * pFilter,
	LPCSTR pszMWFPath
	);

