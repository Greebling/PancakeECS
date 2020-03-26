# PancakeECS
 
**Cache Friendly, Memory Efficient, Multithreaded**

At the heart of this C++17 library is the `ECSManager` class: It takes care of saving all entities and components in contigous memory. It is designed to increase data locality as much as possible while keeping memory usage as low. 

# Usage
An example of how this library might be used can be found in main.cpp

To make use of PancakeECS one only needs to declare a new `Scene`. 
All `GameObjects` that will be instantiated will be created within the scope of the active scene if not declared otherwise. Components can be easily added and removed to `GameObjects` using the `AddComponent<>` and `RemoveComponent<>` methods.

`ComponentViews` allow for easy iteration over ComponentDatas. Multithreading is made easy by means of `parallel_foreach` of a ComponentView.

Implementing new Components is done by inheriting from the `ComponentData` class.
