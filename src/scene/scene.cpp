#include "scene.h"
#include "game/game.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"

using Game::g_state;

namespace Scene {

void init_scene_test() {
    Scene scene;
    scene.name = "test";
    scene.width = 512;
    scene.height = 360;
    scene.background = Renderer::load_texture("scenes/test/backgrounds/bg_field.png");
    
    Prop tree1;
    tree1.position = Vec2(125.0f, 180.0f);
    tree1.size = Vec2(40.0f, 48.0f);
    tree1.texture = Renderer::load_texture("scenes/test/props/prop_tree.png");
    tree1.name = "tree1";
    tree1.pivot = PivotPoint::BOTTOM_CENTER;
    scene.props.push_back(tree1);
    
    Prop tree2;
    tree2.position = Vec2(423.0f, 200.0f);
    tree2.size = Vec2(40.0f, 48.0f);
    tree2.texture = Renderer::load_texture("scenes/test/props/prop_tree.png");
    tree2.name = "tree2";
    tree2.pivot = PivotPoint::BOTTOM_CENTER;
    scene.props.push_back(tree2);
    
    Prop stone;
    stone.position = Vec2(210.0f, 204.0f);
    stone.size = Vec2(24.0f, 24.0f);
    stone.texture = Renderer::load_texture("scenes/test/props/prop_stone.png");
    stone.name = "stone";
    stone.pivot = PivotPoint::BOTTOM_CENTER;
    scene.props.push_back(stone);
    
    Prop chest;
    chest.position = Vec2(373.0f, 291.0f);
    chest.size = Vec2(32.0f, 24.0f);
    chest.texture = Renderer::load_texture("scenes/test/props/prop_chest.png");
    chest.name = "chest";
    chest.pivot = PivotPoint::BOTTOM_CENTER;
    scene.props.push_back(chest);
    
    g_state.scene = scene;
}

}
