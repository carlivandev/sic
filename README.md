# sic
very very wip!

sic is an engine where objects are pre-defined list of components and (s)ystems (i)terate (c)omponents to manipulate their state.

object type creation:
```cpp
struct Player : sic::Object<Player, sic::Component_transform, sic::Component_model> {};
```

component type creation:
```cpp
//model depends on transform, and will cache a pointer to it on creation,
//throws compile time error if object doesn't have a transform
struct Component_model : sic::Component<sic::Component_transform>
{
	sic::Asset_ref<sic::Asset_model> m_model;
};
```

object creation:
```cpp
Player& player = context.create_object<Player>();
```

component iteration:
```cpp
context.for_each<sic::Component_model>
(
	[](sic::Component_model& instance)
	{
		//gets the cached dependency pointer
		instance.get<sic::Component_transform>();
	}
);
```
