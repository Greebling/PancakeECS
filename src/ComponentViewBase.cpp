// Created on 16.02.2020.

#include "ComponentViewBase.h"

ctpl::thread_pool ComponentViewBase::tPool = ctpl::thread_pool(static_cast<int>(std::thread::hardware_concurrency()));