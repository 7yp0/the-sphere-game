#include "scene.h"
#include "game/game.h"
#include "renderer/renderer.h"
#include "renderer/texture_loader.h"

using Game::g_state;

namespace Scene {

void init_scene_test() {
    Scene scene;
    scene.name = "test";
    scene.width = 1280;
    scene.height = 720;
    scene.background = Renderer::load_texture("scenes/test/backgrounds/bg_field.png");
    
    HorizonLine scale_down_horizon;
    scale_down_horizon.y_position = 460.0f;
    scale_down_horizon.scale_gradient = 0.005f;
    scale_down_horizon.depth_scale_inverted = false;
    scene.horizons.push_back(scale_down_horizon);

    HorizonLine scale_up_horizon;
    scale_up_horizon.y_position = 480.0f;
    scale_up_horizon.scale_gradient = 0.005f; 
    scale_up_horizon.depth_scale_inverted = false;
    scene.horizons.push_back(scale_up_horizon);
    
    Prop tree1;
    tree1.position = Vec2(310.0f, 382.0f);
    tree1.size = Vec2(160.0f, 192.0f);
    tree1.texture = Renderer::load_texture("scenes/test/props/prop_tree.png");
    tree1.name = "tree1";
    tree1.pivot = PivotPoint::BOTTOM_CENTER;
    scene.props.push_back(tree1);
    
    Prop tree2;
    tree2.position = Vec2(1024.0f, 461.0f);
    tree2.size = Vec2(160.0f, 192.0f);
    tree2.texture = Renderer::load_texture("scenes/test/props/prop_tree.png");
    tree2.name = "tree2";
    tree2.pivot = PivotPoint::BOTTOM_CENTER;
    scene.props.push_back(tree2);
    
    Prop stone;
    stone.position = Vec2(538.0f, 497.0f);
    stone.size = Vec2(96.0f, 96.0f);
    stone.texture = Renderer::load_texture("scenes/test/props/prop_stone.png");
    stone.name = "stone";
    stone.pivot = PivotPoint::BOTTOM_CENTER;
    scene.props.push_back(stone);
    
    Prop chest;
    chest.position = Vec2(910.0f, 612.0f);
    chest.size = Vec2(128.0f, 96.0f);
    chest.texture = Renderer::load_texture("scenes/test/props/prop_chest.png");
    chest.name = "chest";
    chest.pivot = PivotPoint::BOTTOM_CENTER;
    scene.props.push_back(chest);
    
    g_state.scene = scene;
}

}
