/*
 *      Copyright (C) 2025 Matt Khan
 *      https://github.com/3ll3d00d/mwcapture
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include "version_rev.h"

#define MW_VERSION_MAJOR     0
#define MW_VERSION_MINOR    52
#define MW_VERSION_REVISION  0

/////////////////////////////////////////////////////////
#ifndef ISPP_INCLUDED

#define DO_MAKE_STR(x) #x
#define MAKE_STR(x) DO_MAKE_STR(x)

#if MW_VERSION_BUILD > 0
#define MW_VERSION MW_VERSION_MAJOR.MW_VERSION_MINOR.MW_VERSION_REVISION.MW_VERSION_BUILD-git
#define MW_VERSION_TAG MW_VERSION_MAJOR, MW_VERSION_MINOR, MW_VERSION_REVISION, MW_VERSION_BUILD
#else
#define MW_VERSION MW_VERSION_MAJOR.MW_VERSION_MINOR.MW_VERSION_REVISION
#define MW_VERSION_TAG MW_VERSION_MAJOR, MW_VERSION_MINOR, MW_VERSION_REVISION
#endif

#define MW_VERSION_STR MAKE_STR(MW_VERSION)

#endif
