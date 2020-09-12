#include <iostream>


#ifdef ADD_SUBTITLE
#include "add_subtitle.h"
#endif

int main(int argc, char *argv[])
{

    std::cout<<"execute main."<<std::endl;

#ifdef ADD_SUBTITLE
    // media_tool SampleVideo_1280x720_20mb.mkv "add subtitle"
    std::cout<<"exec add subtitle"<<std::endl;
    add_subtitle_main(argv[1], argv[2]);
#endif

    return 0;
}
