#include <chrono>
#include <cstddef>
#include <iostream>

#include "../../../include/gtest/gtest.h"
#include "SchedulingGroup.h"
#include "FiberEntity.h"
#include "Waitable.h"


using namespace tinyRPC::fiber::detail;
using namespace std;
int main(){
  constexpr uint32_t workerNum = 1;
  auto sg = std::make_unique<SchedulingGroup>(workerNum);
  std::thread workers[workerNum];
  TimerWorker dummy(sg.get());
  sg->SetTimerWorker(&dummy); 
  
  CreateFiberEntity(sg.get(), [&] { }, std::make_shared<ExitBarrier>());

  cout << "Func" << endl;
  int a = 1;
  int b = 2;

  cout << a + b << endl;
} 