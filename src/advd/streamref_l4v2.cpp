#include "streamref.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <experimental/filesystem>

extern "C"
{
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}


namespace fs = std::experimental::filesystem;

namespace advd {
    std::string StreamRef::id() const {
        
        return std::any_cast<std::string>(m_handle);
    }
    std::string StreamRef::label() const {
        return std::any_cast<std::string>(m_handle);
    }
    std::vector<StreamRef> StreamRef::enumerate() {
        std::vector<StreamRef> tr;

        for(const auto & current_file : fs::directory_iterator("/dev") )  {
            if(current_file.path().string().substr(0,10) == "/dev/video") {
                std::string name = current_file.path().string();

                // Check if camera actually works
                AVFormatContext* format_context = avformat_alloc_context();
                AVInputFormat* format = av_find_input_format("l4v2");
                AVDictionary* dictionary = NULL;
			    av_dict_set(&dictionary, "framerate", "30", 0);
                
                if(avformat_open_input(&format_context, name.c_str(), format, &dictionary) == 0)
                    tr.push_back(StreamRef {name});

                avformat_close_input(&format_context);
            }
        }
        return tr;
    }
    StreamRef::~StreamRef() {
        
    }
}
