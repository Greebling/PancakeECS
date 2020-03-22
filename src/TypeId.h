#pragma once


typedef unsigned short ComponentId;

struct BaseTypeId
{
protected:
	inline static ComponentId lastId = 0;
};

template<typename T>
struct TypeId : public BaseTypeId
{
public:
	static ComponentId GetId()
	{
		static ComponentId id = lastId++;
		return id;
	}
};