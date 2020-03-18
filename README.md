# PancakeECS
 
**Cache Friendly, Memory Efficient, Multithreaded**

At the heart of this C++17 library is the `ECSManager` class: It takes care of saving all components in their respective `ComponentVectors`. It is designed to increase data locality as much as possible while keeping memory usage as low as possible. 

# Usage
To make use of PancakeECS one only needs to declare a new `Scene`. All `GameObjects` that will be declared will be created within the scope of the active scene. Components can be easily added and removed to `GameObjects` using the `AddComponent<>` and `RemoveComponent<>` methods. 

`ComponentViews` allow for easy iteration over ComponentDatas. Multithreading is made easy by means of `parallel_foreach` of a ComponentView.

Implementing new Components is done by inheriting from the `ComponentData` class.
