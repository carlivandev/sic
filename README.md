# impuls
very very wip engine
an engine where objects are pre-defined list of components and systems iterate components to manipulate their state.

object type creation:
```cpp
struct player : i_object<component_transform, component_model> {};
```

component type creation:
```cpp
//model depends on transform, and will cache a pointer to it on creation,
//throws compile time error if object doesn't have a transform
struct component_model : i_component<component_transform>
{
	asset_ref<asset_model> m_model;
	std::vector<asset_ref<asset_material>> m_material_overrides;
};
```

object creation:
```cpp
auto& player_instance = context.create_object<player>();
```

component iteration:
```cpp
for (auto&& instance : context.each<component_model>())
{
	//gets the cached dependency pointer
	instance.get<component_transform>();
}
```
