#include <iostream>
#include "parallelm.hpp"


int main(int argc, char** argv)
{
    std::string path = "/home/daoio01/parallelmachineoridata/1020.txt";
    std::string result_path = "/home/daoio01/parallelmachineres/999.txt";
    result_path = "/home/daoio01/test.txt";
    int Timelimit = 300;
    int Threads = 1;
    if (argc >= 4)
    {
        path = argv[1];                      //算例绝对路径
        result_path = argv[2];                //结果保存绝对路径
        Timelimit = std::stoi(argv[3]);  //时间限制
        Threads = std::stoi(argv[4]);    //线程数
    }
    auto parallelm = ParallelM(result_path);
    parallelm.loadinstancebyfile(path);
    TIMER::Start(Timelimit);
//    parallelm.end2endsolve(Timelimit,Threads);
    parallelm.run();
    return 0;
}

