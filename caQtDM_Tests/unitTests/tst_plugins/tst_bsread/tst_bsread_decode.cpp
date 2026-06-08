#include "tst_bsread_decode.h"
#include "zmq.h"

void Testbsread_Decode::initTestCase()
{
    // code to be executed before the first test function

}

void Testbsread_Decode::init()
{
    // code to be executed before each test function
    void* zmqcontext = zmq_ctx_new();
    bsread_Decode *decode = new bsread_Decode(zmqcontext, "tcp://127.0.0.1:5555");
}

void Testbsread_Decode::cleanupTestCase()
{
    // code to be executed after the last test function

}

void Testbsread_Decode::cleanup()
{
    // code to be executed after each test function

}

void Testbsread_Decode::test1()
{

}

void Testbsread_Decode::test2()
{

}

void Testbsread_Decode::test3()
{

}

void Testbsread_Decode::test4()
{

}

