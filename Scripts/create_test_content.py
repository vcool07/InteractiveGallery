"""Placeholder gallery for testing the POI, material and AI painting systems.

Creates /Game/GalleryCore/TestContent (colour + canvas materials) and builds, in /Game/Maps/Main:
  - a two-hall gallery (sculpture hall by the entrance, painting hall behind a partition)
  - 3 sculptures on pedestals and 5 AI painting frames, each with a POI target
  - one wall-finish switcher shared by every wall
  - a PlayerStart at the entrance and a Spawn_TopView point over the middle

No ceiling, so Top view can look in. Safe to re-run: everything it placed is tagged and replaced.
Delete the tagged actors and the TestContent folder once real art exists.

Headless:  UnrealEditor-Cmd.exe InteractiveGallery.uproject -run=pythonscript -script="<path to this file>"
In editor: Tools > Execute Python Script
"""

import unreal

ROOT = "/Game/GalleryCore/TestContent"
MAP = "/Game/Maps/Main"
TAG = "GalleryTestContent"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
assets = unreal.EditorAssetLibrary
mat_lib = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log(f"[TestContent] {msg}")


# ------------------------------------------------------------------ materials

def create_material(name):
    path = f"{ROOT}/{name}"
    if assets.does_asset_exist(path):
        return assets.load_asset(path), False
    return asset_tools.create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew()), True


def add_param(mat, cls, name, default, x, y):
    node = mat_lib.create_material_expression(mat, cls, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default)
    return node


def build_color_material():
    mat, created = create_material("M_GalleryColor")
    if created:
        color = add_param(mat, unreal.MaterialExpressionVectorParameter, "Color", unreal.LinearColor(0.8, 0.8, 0.8, 1.0), -400, -200)
        metallic = add_param(mat, unreal.MaterialExpressionScalarParameter, "Metallic", 0.0, -400, 0)
        roughness = add_param(mat, unreal.MaterialExpressionScalarParameter, "Roughness", 0.5, -400, 150)
        mat_lib.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
        mat_lib.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
        mat_lib.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
        mat_lib.recompile_material(mat)
        assets.save_loaded_asset(mat)
        log("Created M_GalleryColor")
    return mat


def build_canvas_material():
    """Texture parameter 'Artwork' (what AIPaintingFrame sets) with a little glow so paintings read well."""
    mat, created = create_material("M_PaintingCanvas")
    if created:
        mat.set_editor_property("two_sided", True)
        art = mat_lib.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -600, -100)
        art.set_editor_property("parameter_name", "Artwork")
        art.set_editor_property("texture", assets.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
        glow = add_param(mat, unreal.MaterialExpressionScalarParameter, "Glow", 0.12, -600, 200)
        roughness = add_param(mat, unreal.MaterialExpressionScalarParameter, "Roughness", 0.85, -600, 350)
        emissive = mat_lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 150)
        mat_lib.connect_material_expressions(art, "RGB", emissive, "A")
        mat_lib.connect_material_expressions(glow, "", emissive, "B")
        mat_lib.connect_material_property(art, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
        mat_lib.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        mat_lib.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
        mat_lib.recompile_material(mat)
        assets.save_loaded_asset(mat)
        log("Created M_PaintingCanvas")
    return mat


# name: (linear colour, metallic, roughness)
FINISHES = {
    "SeaSalt":    ((0.86, 0.84, 0.80), 0.0, 0.85),
    "Lagoon":     ((0.03, 0.30, 0.34), 0.0, 0.70),
    "Terracotta": ((0.55, 0.18, 0.08), 0.0, 0.90),
    "Sand":       ((0.72, 0.58, 0.38), 0.0, 0.95),
    "Pearl":      ((0.90, 0.87, 0.84), 0.3, 0.20),
    "Gold":       ((1.00, 0.71, 0.29), 1.0, 0.25),
    "Coral":      ((0.95, 0.33, 0.28), 0.0, 0.55),
    "Obsidian":   ((0.02, 0.02, 0.025), 0.0, 0.08),
    "Marble":     ((0.80, 0.80, 0.78), 0.0, 0.30),
    "Basalt":     ((0.07, 0.07, 0.07), 0.0, 0.80),
    "Walnut":     ((0.10, 0.045, 0.02), 0.0, 0.55),
    "Brass":      ((0.80, 0.60, 0.30), 1.0, 0.35),
    "Driftwood":  ((0.45, 0.40, 0.34), 0.0, 0.80),
    "Ink":        ((0.015, 0.015, 0.02), 0.0, 0.40),
}


def build_finish(parent, name):
    path = f"{ROOT}/MI_{name}"
    if assets.does_asset_exist(path):
        mi = assets.load_asset(path)
    else:
        mi = asset_tools.create_asset(f"MI_{name}", ROOT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    (r, g, b), metallic, roughness = FINISHES[name]
    mat_lib.set_material_instance_parent(mi, parent)
    mat_lib.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(r, g, b, 1.0))
    mat_lib.set_material_instance_scalar_parameter_value(mi, "Metallic", metallic)
    mat_lib.set_material_instance_scalar_parameter_value(mi, "Roughness", roughness)
    mat_lib.update_material_instance(mi)
    assets.save_loaded_asset(mi)
    return mi


def variants(mi, names):
    result = []
    for name in names:
        (r, g, b), _, _ = FINISHES[name]
        v = unreal.MaterialVariant()
        v.set_editor_property("display_name", unreal.Text(name))
        v.set_editor_property("material", mi[name])
        v.set_editor_property("swatch_color", unreal.LinearColor(r, g, b, 1.0))
        result.append(v)
    return result


def set_variants(component, label, variant_list, linked_actors=None):
    component.set_editor_property("slot_label", unreal.Text(label))
    component.set_editor_property("variants", variant_list)
    component.set_editor_property("initial_variant", 0)
    if linked_actors:
        component.set_editor_property("linked_actors", linked_actors)


# ------------------------------------------------------------------ level helpers

def floor_top_z(actor_sys):
    """Top of the largest flat static mesh under the origin (the template floor), else 0."""
    best, best_area = 0.0, 0.0
    for actor in actor_sys.get_all_level_actors():
        if not isinstance(actor, unreal.StaticMeshActor) or actor.actor_has_tag(TAG):
            continue
        origin, extent = actor.get_actor_bounds(False)
        area = extent.x * extent.y
        covers_origin = abs(origin.x) <= extent.x and abs(origin.y) <= extent.y
        is_flat = extent.z < 200 and extent.x < 100000  # skip sky spheres and other huge meshes
        if covers_origin and is_flat and area > best_area:
            best, best_area = origin.z + extent.z, area
    return best


def rot(yaw=0.0, pitch=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)


def spawn(actor_sys, cls, label, location, rotation=None, extra_tags=()):
    actor = actor_sys.spawn_actor_from_class(cls, location, rotation or rot())
    actor.set_actor_label(label)
    actor.tags = [TAG, *extra_tags]
    return actor


def spawn_box(actor_sys, cls, label, center, size, material):
    """A BasicShapes cube scaled to `size` (cm). cls: StaticMeshActor or GalleryVariantMeshActor."""
    actor = spawn(actor_sys, cls, label, center)
    actor.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    mesh = actor.static_mesh_component if cls == unreal.StaticMeshActor else actor.get_editor_property("mesh")
    mesh.set_static_mesh(assets.load_asset("/Engine/BasicShapes/Cube"))
    mesh.set_material(0, material)
    return actor


def spawn_poi(actor_sys, order, label, pivot, yaw, pitch, title, description, targets,
              distance=320.0, min_distance=140.0, max_distance=700.0, marker_height=80.0):
    poi = spawn(actor_sys, unreal.POITarget, label, pivot, rot(yaw=yaw, pitch=pitch))
    poi.set_editor_property("display_name", unreal.Text(title))
    poi.set_editor_property("description", unreal.Text(description))
    poi.set_editor_property("order", order)
    poi.set_editor_property("view_distance", distance)
    poi.set_editor_property("min_view_distance", min_distance)
    poi.set_editor_property("max_view_distance", max_distance)
    poi.set_editor_property("marker_offset", unreal.Vector(0, 0, marker_height))
    poi.set_editor_property("material_targets", targets)
    return poi


# ------------------------------------------------------------------ layout
#
#            y = +700  ───────────────── north wall ─────────────────
#            │  sculpture hall          │      painting hall          │
#   entrance    (pedestals x=-450)      │  (frames on every wall)      east
#   (door)   │                       opening                         │ wall
#            │                          │                              │
#            y = -700  ───────────────── south wall ─────────────────
#          x = -900                   x = 0                        x = 900

HALF_X, HALF_Y = 900.0, 700.0
WALL_T, WALL_H = 20.0, 400.0
DOOR_W, OPENING_W = 300.0, 400.0


def build_walls(actor_sys, z, wall_material):
    zc = z + WALL_H / 2
    walls = []

    def wall(label, x0, y0, x1, y1, cls=unreal.StaticMeshActor):
        size = (max(abs(x1 - x0), WALL_T), max(abs(y1 - y0), WALL_T), WALL_H)
        center = unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, zc)
        actor = spawn_box(actor_sys, cls, label, center, size, wall_material)
        walls.append(actor)
        return actor

    ext = HALF_X + WALL_T / 2
    master = wall("Wall_East", HALF_X, -HALF_Y, HALF_X, HALF_Y, unreal.GalleryVariantMeshActor)
    wall("Wall_North", -ext, HALF_Y, ext, HALF_Y)
    wall("Wall_South", -ext, -HALF_Y, ext, -HALF_Y)
    wall("Wall_West_N", -HALF_X, DOOR_W / 2, -HALF_X, HALF_Y)
    wall("Wall_West_S", -HALF_X, -HALF_Y, -HALF_X, -DOOR_W / 2)
    wall("Partition_N", 0, OPENING_W / 2, 0, HALF_Y)
    wall("Partition_S", 0, -HALF_Y, 0, -OPENING_W / 2)

    # Lintel over the entrance so the doorway reads as a door
    lintel_h = 100.0
    walls.append(spawn_box(actor_sys, unreal.StaticMeshActor, "Wall_West_Lintel",
                           unreal.Vector(-HALF_X, 0, z + WALL_H - lintel_h / 2), (WALL_T, DOOR_W, lintel_h), wall_material))

    return master, [w for w in walls if w is not master]


FRAMES = [
    # label, (x, y) on the wall surface, facing yaw, canvas (w, h), title, prompt
    ("Frame_East", (HALF_X - WALL_T / 2, 0), 180, (260, 170), "Tidal Light",
     "a vast turquoise sea at golden hour seen from a garden of palms and bougainvillea"),
    ("Frame_North", (450, HALF_Y - WALL_T / 2), -90, (170, 125), "Shell House",
     "an elegant gallery building shaped like a nautilus seashell on a cliff above the ocean"),
    ("Frame_South", (450, -HALF_Y + WALL_T / 2), 90, (170, 125), "Night Swim",
     "bioluminescent waves on a quiet beach under a starry sky"),
    ("Frame_Partition_N", (WALL_T / 2, 450), 0, (120, 160), "Coral Garden",
     "a lush seaside garden with coral-coloured flowers and stone paths, portrait"),
    ("Frame_Partition_S", (WALL_T / 2, -450), 0, (120, 160), "Driftwood",
     "a sculptural piece of driftwood on white sand, soft morning light, portrait"),
]

SCULPTURES = [
    # shape, y, rotation, title, description
    ("Sphere", -380, rot(), "Pearl Study", "A placeholder sculpture. Try the finishes, then drag to orbit."),
    ("Cone", 0, rot(), "Spire", "Placeholder for a shell-inspired spire."),
    ("Cube", 380, rot(yaw=35, roll=45), "Tilted Block", "Placeholder exhibit. Left / Right arrows visit the neighbours."),
]


def main():
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    actor_sys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    for actor in actor_sys.get_all_level_actors():
        if actor.actor_has_tag(TAG):
            actor_sys.destroy_actor(actor)

    color = build_color_material()
    canvas_material = build_canvas_material()
    mi = {name: build_finish(color, name) for name in FINISHES}

    z = floor_top_z(actor_sys)
    log(f"Floor top at Z={z:.1f}")

    # Entrance + Top view pivot
    if not any(isinstance(a, unreal.PlayerStart) for a in actor_sys.get_all_level_actors()):
        spawn(actor_sys, unreal.PlayerStart, "PlayerStart", unreal.Vector(-HALF_X - 60, 0, z + 100))
    spawn(actor_sys, unreal.TargetPoint, "Spawn_TopView", unreal.Vector(0, 0, z), extra_tags=("Spawn_TopView",))

    # Walls: the east wall carries the finish switcher for all of them
    wall_master, other_walls = build_walls(actor_sys, z, mi["SeaSalt"])
    set_variants(wall_master.get_editor_property("material_variants"), "Wall finish",
                 variants(mi, ("SeaSalt", "Lagoon", "Terracotta", "Sand")), other_walls)

    order = 0
    sculpture_finishes = variants(mi, ("Pearl", "Gold", "Coral", "Obsidian"))
    for shape, y, rotation, title, description in SCULPTURES:
        x = -450
        pedestal = spawn_box(actor_sys, unreal.StaticMeshActor, f"Pedestal_{shape}",
                             unreal.Vector(x, y, z + 50), (70, 70, 100), mi["Marble"])
        pedestal.static_mesh_component.set_static_mesh(assets.load_asset("/Engine/BasicShapes/Cylinder"))

        exhibit_z = z + 100 + 40
        exhibit = spawn(actor_sys, unreal.GalleryVariantMeshActor, f"Exhibit_{shape}", unreal.Vector(x, y, exhibit_z), rotation)
        exhibit.set_actor_scale3d(unreal.Vector(0.6, 0.6, 0.6))
        exhibit.get_editor_property("mesh").set_static_mesh(assets.load_asset(f"/Engine/BasicShapes/{shape}"))
        set_variants(exhibit.get_editor_property("material_variants"), "Sculpture finish", sculpture_finishes)

        spawn_poi(actor_sys, order, f"POI_{order + 1:02d}_{shape}", unreal.Vector(x, y, exhibit_z), 0, -12,
                  title, description, [exhibit, wall_master])
        order += 1

    frame_finishes = variants(mi, ("Walnut", "Brass", "Driftwood", "Ink"))
    for label, (wx, wy), yaw, (w, h), title, prompt in FRAMES:
        canvas_z = z + 170
        facing = unreal.MathLibrary.get_forward_vector(rot(yaw=yaw))
        depth = 6.0
        location = unreal.Vector(wx + facing.x * depth / 2, wy + facing.y * depth / 2, canvas_z)

        frame = spawn(actor_sys, unreal.AIPaintingFrame, label, location, rot(yaw=yaw))
        frame.set_editor_property("title", unreal.Text(title))
        frame.set_editor_property("default_prompt", prompt)
        frame.set_editor_property("canvas_width", float(w))
        frame.set_editor_property("canvas_height", float(h))
        frame.set_editor_property("frame_depth", depth)
        frame.set_editor_property("canvas_material", canvas_material)
        frame.set_editor_property("frame_material", mi["Walnut"])
        set_variants(frame.get_editor_property("frame_variants"), "Frame finish", frame_finishes)

        # The camera looks into the frame, so the POI faces the opposite way
        distance = max(w, h) * 1.6 + 60
        spawn_poi(actor_sys, order, f"POI_{order + 1:02d}_{label}", location, yaw + 180, -3,
                  title, "AI painting. Type a prompt below and press Paint; ComfyUI paints it on this canvas.",
                  [frame, wall_master], distance=distance, min_distance=120.0, max_distance=distance * 2,
                  marker_height=h / 2 + 40)
        order += 1

    unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
    log(f"Built gallery: {len(SCULPTURES)} sculptures, {len(FRAMES)} AI frames, {order} POIs. Saved Main")


main()
