/**
 * $LicenseInfo:firstyear=2014&license=fsviewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (C) 2014, Nicky Dasmijn
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
 * http://www.firestormviewer.org
 * $/LicenseInfo$
 */

#ifndef NDLEAPEXAMPLETOOL_H
#define NDLEAPEXAMPLETOOL_H

#pragma once

#include "fsleaptool.h"

class LLViewerObject;

namespace nd
{
	namespace leap
	{
		class ExampleTool: public Tool
		{
			int mHands;
			int mFingers;
		public:
			virtual ~ExampleTool(){}

			virtual void onLeapFrame( Leap::Frame const& );
			virtual void onRenderFrame( Leap::Frame const& );
			virtual void render();
			virtual std::string getDebugString();
			virtual std::string getName();
			virtual S32 getId();
		};
	}
}

#endif
