#pragma once
#include <algorithm>
#include <list>
#include <optional>

namespace h5pp {
    class File;
    namespace counter {
        class ActiveFileCounter {
            public:
            friend class h5pp::File;
            inline static size_t                 getCount() { return openFiles.size(); }
            inline static std::list<std::string> getOpenFiles() { return openFiles; }
            inline static std::string            OpenFileNames() {
                std::string names;
                for(auto &file : openFiles) { names += file + " "; }
                return names;
            }

            private:
            inline static std::list<std::string> openFiles;
            inline static void                   incrementCounter(const std::string &fileName) { openFiles.push_front(fileName); }
            inline static void                   decrementCounter(const std::string &fileName) {
                auto iter = std::find(openFiles.begin(), openFiles.end(), fileName);
                if(iter != openFiles.end()) { openFiles.erase(iter); }
            }
        };

    }
}
