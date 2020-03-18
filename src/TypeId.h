// Created on 21.12.2019.

#pragma once



typedef unsigned short ComponentId;

struct BaseTypeId {
protected:
    static ComponentId lastId;
};

template<typename T>
struct TypeId : public BaseTypeId {
public:
    static ComponentId GetId() {
        static auto id = lastId++;
        return id;
    }
};


