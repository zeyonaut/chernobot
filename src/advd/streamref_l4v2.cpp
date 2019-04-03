#include "streamref.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <experimental/filesystem>

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
                tr.push_back(StreamRef {current_file.path().string()});
            }
        }
        return tr;
    }
    StreamRef::~StreamRef() {
        
    }
}
