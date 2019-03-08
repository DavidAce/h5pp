//
// Created by david on 2019-03-08.
//

#ifndef H5PP_COUNTER_H
#define H5PP_COUNTER_H
#include <optional>
#include <list>
#include <algorithm>



namespace h5pp{
    class File;
    namespace Counter {
//        class ActiveFileCounter {
//        public:
//            friend class h5pp::File;
//            inline static size_t getCount() { return fileCount.value(); }
//        private:
//            inline static std::optional<size_t> fileCount;
//
//
//            inline static void incrementCounter() {
//                if (!fileCount) {
//                    fileCount = 0;
//                }
//                fileCount.value()++;
//            }
//
//            inline static void decrementCounter() { fileCount.value()--; }
//
//        };

        class ActiveFileCounter {
        public:
            friend class h5pp::File;
            inline static size_t getCount() { return openFiles.size(); }
            inline static std::list<std::string> getOpenFiles() { return openFiles; }
            inline static std::string OpenFileNames() {
                std::string names;
                for(auto &file : openFiles){
                    names += file + " ";
                }
                return names;
            }

        private:
            inline static std::list<std::string> openFiles;
            inline static void incrementCounter(const std::string fileName) {
                openFiles.push_front(fileName);
            }
            inline static void decrementCounter(const std::string fileName) {
                auto iter = std::find(openFiles.begin(), openFiles.end(), fileName);
                if (iter != openFiles.end())
                {
                    openFiles.erase(iter);
                }
            }

        };


    }
}

#endif //H5PP_COUNTER_H
