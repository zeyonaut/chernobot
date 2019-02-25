//extern "C"
//{
#import <AVFoundation/AVFoundation.h> 
//}
#import "streamref.hpp"

#include <iostream>
#include <vector>
#include <any>

namespace advd
{
	StreamRef::~StreamRef()
	{
		[std::any_cast<AVCaptureDevice *>(m_handle) release];
	}

	std::string StreamRef::id () const
	{
		return std::string {[[std::any_cast<AVCaptureDevice *>(m_handle) uniqueID] UTF8String]};
	}

	std::string StreamRef::label () const
	{
		return std::string {[[std::any_cast<AVCaptureDevice *>(m_handle) localizedName] UTF8String]};
	}

	std::vector<StreamRef> StreamRef::enumerate ()
	{
		std::vector<StreamRef> videostreams;
		NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];

		for (AVCaptureDevice *device in devices)
		{
			videostreams.push_back(StreamRef {device});
		}

		[devices release];

		return videostreams;
	}
}