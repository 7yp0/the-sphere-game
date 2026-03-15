#define GL_SILENCE_DEPRECATION
#import "mac_window.h"
#include "renderer/renderer.h"
#include "types.h"
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#include <cstdio>

// Forward declarations
namespace Platform {
    void set_mouse_pos(Vec2 pos);
    void set_mouse_clicked(bool clicked);
    void set_mouse_down(bool down);
}

@class OpenGLView;

@interface KeyboardView : NSView
@property (nonatomic, strong) OpenGLView* glView;
@end

@implementation KeyboardView
{
    NSTrackingArea* _trackingArea;
}


- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    if (_trackingArea) {
        [self removeTrackingArea:_trackingArea];
        _trackingArea = nil;
    }
    _trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                 options:(NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect)
                                                   owner:self
                                                userInfo:nil];
    [self addTrackingArea:_trackingArea];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)isOpaque {
    return YES;
}

- (void)viewDidMoveToWindow {
    if (self.window) {
        [self.window makeFirstResponder:self];
    }
    // Hintergrundfarbe der View auf Schwarz setzen
    self.layer.backgroundColor = [NSColor blackColor].CGColor;
}

- (void)drawRect:(NSRect)dirtyRect {
    [[NSColor blackColor] setFill];
    NSRectFill(dirtyRect);
    [self.glView display];
}

- (void)keyDown:(NSEvent *)event {
    // Option+Enter (Return) toggles fullscreen
    if (event.keyCode == 36 && ([event modifierFlags] & NSEventModifierFlagOption)) {
        Platform::toggle_fullscreen();
        return;
    }
    Platform::set_key_pressed(event.keyCode, true);
}

- (void)keyUp:(NSEvent *)event {
    Platform::set_key_pressed(event.keyCode, false);
}

- (void)mouseDown:(NSEvent *)event {
    Platform::set_mouse_clicked(true);
    Platform::set_mouse_down(true);
}

- (void)mouseUp:(NSEvent *)event {
    Platform::set_mouse_down(false);
}

- (void)rightMouseDown:(NSEvent *)event {
    Platform::set_mouse_right_clicked(true);
}

- (void)mouseDragged:(NSEvent *)event {
    NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
    Platform::set_mouse_pos(Vec2(loc.x, Platform::get_window_height() - loc.y));
}

- (void)mouseMoved:(NSEvent *)event {
    NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
    
    // Flip Y: use the stored window height (not view height which might differ)
    Platform::set_mouse_pos(Vec2(loc.x, Platform::get_window_height() - loc.y));
}

- (void)scrollWheel:(NSEvent *)event {
    // Accumulate scroll delta (positive = scroll up, negative = scroll down)
    Platform::set_scroll_delta([event scrollingDeltaY]);
}

@end

@interface OpenGLView : NSOpenGLView
@end

@implementation OpenGLView

- (BOOL)isOpaque {
    return YES;
}

// Letterbox-Viewport-Berechnung mit Integer-Scaling auf 16:9 (Vielfaches von 320x180)
- (void)drawRect:(NSRect)dirtyRect {
    // 1. Komplett schwarz füllen
    [[NSColor blackColor] setFill];
    NSRectFill(dirtyRect);

    // 2. Letterbox: Erzwinge IMMER 16:9 (Vielfaches von 320x180)
    const int baseW = 320;
    const int baseH = 180;
    CGFloat viewW = self.bounds.size.width;
    CGFloat viewH = self.bounds.size.height;
    CGFloat aspect = 16.0/9.0;
    // Berechne maximalen 16:9-Bereich, der in die View passt
    CGFloat targetW = viewW;
    CGFloat targetH = viewW / aspect;
    if (targetH > viewH) {
        targetH = viewH;
        targetW = viewH * aspect;
    }
    // Integer-Scaling auf Vielfaches von 320x180
    int scale = (int)fmin(floor(targetW / baseW), floor(targetH / baseH));
    if (scale < 1) scale = 1;
    int intW = baseW * scale;
    int intH = baseH * scale;
    CGFloat offsetX = (viewW - intW) * 0.5;
    CGFloat offsetY = (viewH - intH) * 0.5;
    NSRect gameRect = NSMakeRect(offsetX, offsetY, intW, intH);

    // 3. OpenGL-Viewport setzen (Renderer bekommt diese Info!)
    Renderer::set_viewport((uint32_t)intW, (uint32_t)intH);

    // 4. OpenGL-Viewport verschieben (Letterbox):
    glViewport((GLint)offsetX, (GLint)offsetY, (GLsizei)intW, (GLsizei)intH);

    // 5. Spielfeld rendern (eigentliches Spiel-Rendering)
    [super drawRect:gameRect];
}

@end

@interface WindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) bool* shouldClosePtr;
@end

@implementation WindowDelegate
- (BOOL)windowShouldClose:(NSWindow *)sender {
    *self.shouldClosePtr = true;
    return YES;
}
@end

namespace Platform {

static NSWindow* g_window = nil;
static NSOpenGLContext* g_glContext = nil;
static KeyboardView* g_keyboardView = nil;
static bool g_shouldClose = false;
static Vec2 g_mousePos = Vec2(0.0f, 0.0f);
static bool g_mouseClicked = false;
static bool g_mouseRightClicked = false;
static bool g_mouseDown = false;
static float g_scrollDelta = 0.0f;
static bool g_keys[256] = {};
static uint32_t g_window_width = 0;
static uint32_t g_window_height = 0;
static bool g_fullscreen = false;

// Forward declaration
void toggle_fullscreen();

void set_key_pressed(int key_code, bool pressed) {
    if (key_code < 256) {
        g_keys[key_code] = pressed;
    }
}

bool init_window(const WindowConfig& config)
{
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        
        // CRITICAL: Activate app BEFORE any window operations
        [app activateIgnoringOtherApps:YES];

        NSUInteger style = NSWindowStyleMaskTitled |
                           NSWindowStyleMaskClosable |
                           NSWindowStyleMaskResizable;

        NSRect rect = NSMakeRect(0, 0, config.width, config.height);

                g_window = [[NSWindow alloc] initWithContentRect:rect
                                                                                                styleMask:style
                                                                                                    backing:NSBackingStoreBuffered
                                                                                                        defer:NO];
                [g_window setTitle:[NSString stringWithUTF8String:config.title]];
                // Fensterhintergrundfarbe auf Schwarz
                [g_window setBackgroundColor:[NSColor blackColor]];
                [g_window setOpaque:YES];
                [g_window setHasShadow:NO];

        // Create OpenGL pixel format
        NSOpenGLPixelFormatAttribute attrs[] = {
            NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
            NSOpenGLPFAColorSize, 24,
            NSOpenGLPFAAlphaSize, 8,
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFADepthSize, 24,
            0
        };

        NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        
        // Create OpenGL view with the pixel format
        OpenGLView* glView = [[OpenGLView alloc] initWithFrame:rect pixelFormat:pixelFormat];
        [glView setWantsBestResolutionOpenGLSurface:NO];  // Disable Retina scaling - use logical pixels
        
        // Create keyboard handling view (TOP LEVEL)
        g_keyboardView = [[KeyboardView alloc] initWithFrame:rect];
        g_keyboardView.glView = glView;
        
        // Add GL view as subview of keyboard view
        [g_keyboardView addSubview:glView];
        
        // Add tracking area for mouse movement
        NSTrackingArea* trackingArea = [[NSTrackingArea alloc]
            initWithRect:rect
            options:(NSTrackingMouseMoved | NSTrackingActiveAlways)
            owner:g_keyboardView
            userInfo:nil];
        [g_keyboardView addTrackingArea:trackingArea];
        
        // Get the context from the view and make it current
        g_glContext = [glView openGLContext];
        [g_glContext makeCurrentContext];

        // Set swap interval (vsync)
        GLint swapInt = 1;
        [g_glContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

        // Set keyboard view as content view
        [g_window setContentView:g_keyboardView];
        
        // Get actual content size (analogous to Windows GetClientRect)
        NSRect contentBounds = [[g_window contentView] bounds];
        g_window_width = (uint32_t)contentBounds.size.width;
        g_window_height = (uint32_t)contentBounds.size.height;
        
        printf("[MAC] Requested: %u x %u, Actual content: %u x %u\n",
               config.width, config.height, g_window_width, g_window_height);
        fflush(stdout);
        
        // CRITICAL: Make first responder IMMEDIATELY after setContentView
        [g_window makeFirstResponder:g_keyboardView];

        // Set up window delegate for close events
        WindowDelegate* delegate = [[WindowDelegate alloc] init];
        delegate.shouldClosePtr = &g_shouldClose;
        [g_window setDelegate:delegate];

        [app finishLaunching];
        [g_window makeKeyAndOrderFront:nil];
        return true;
    }
}

void swap_buffers()
{
    @autoreleasepool {
        [g_glContext makeCurrentContext];
        [g_glContext flushBuffer];
    }
}

bool window_should_close()
{
    @autoreleasepool {
        NSEvent* event;
        do {
            event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                       untilDate:[NSDate distantPast]
                                          inMode:NSDefaultRunLoopMode
                                         dequeue:YES];
            if (event) {
                [NSApp sendEvent:event];
            }
        } while (event != nil);

        [NSApp updateWindows];
        return g_shouldClose;
    }
}

void shutdown()
{
    @autoreleasepool {
        [g_window close];
        g_window = nil;
        g_glContext = nil;
    }
}

Vec2 get_mouse_pos()
{
    return g_mousePos;
}

uint32_t get_window_width()
{
    if (g_fullscreen && g_window) {
        NSScreen* screen = [g_window screen] ?: [NSScreen mainScreen];
        NSRect screenFrame = [screen frame];
        return (uint32_t)screenFrame.size.width;
    }
    return g_window_width;
}

uint32_t get_window_height()
{
    if (g_fullscreen && g_window) {
        NSScreen* screen = [g_window screen] ?: [NSScreen mainScreen];
        NSRect screenFrame = [screen frame];
        return (uint32_t)screenFrame.size.height;
    }
    return g_window_height;
}

bool mouse_clicked()
{
    bool clicked = g_mouseClicked;
    g_mouseClicked = false;
    return clicked;
}

bool key_pressed(int key_code)
{
    if (key_code < 256) {
        return g_keys[key_code];
    }
    return false;
}

bool shift_down()
{
    return ([NSEvent modifierFlags] & NSEventModifierFlagShift) != 0;
}

bool control_down()
{
    // On Mac, use Command key (more natural for Mac users)
    return ([NSEvent modifierFlags] & NSEventModifierFlagCommand) != 0;
}

void set_mouse_pos(Vec2 pos)
{
    g_mousePos = pos;
}

void set_mouse_clicked(bool clicked)
{
    g_mouseClicked = clicked;
}

void set_mouse_down(bool down)
{
    g_mouseDown = down;
}

bool mouse_down()
{
    return g_mouseDown;
}

bool mouse_right_clicked()
{
    bool clicked = g_mouseRightClicked;
    g_mouseRightClicked = false;
    return clicked;
}

void set_mouse_right_clicked(bool clicked)
{
    g_mouseRightClicked = clicked;
}

float scroll_delta()
{
    float delta = g_scrollDelta;
    g_scrollDelta = 0.0f;  // Consume on read
    return delta;
}

void set_scroll_delta(float delta)
{
    g_scrollDelta += delta;  // Accumulate in case of multiple events per frame
}

void show_system_cursor(bool show)
{
    if (show) {
        [NSCursor unhide];
    } else {
        [NSCursor hide];
    }
}

void toggle_fullscreen()
{
    @autoreleasepool {
        if (!g_fullscreen) {
            // Borderless fullscreen: cover the ENTIRE screen
            NSScreen* screen = [g_window screen] ?: [NSScreen mainScreen];
            NSRect screenFrame = [screen frame];
            // Remove window decorations and cover full screen
            [g_window setStyleMask:NSWindowStyleMaskBorderless];
            [g_window setFrame:screenFrame display:YES animate:NO];
            [g_window setLevel:NSFloatingWindowLevel];  // Keep on top
            [g_window setBackgroundColor:[NSColor blackColor]];
            [g_window setOpaque:YES];
            // Menüleiste und Dock ausblenden
            [NSApp setPresentationOptions:(NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar)];
            g_window_width = (uint32_t)screenFrame.size.width;
            g_window_height = (uint32_t)screenFrame.size.height;
            // OpenGLView auf neue Größe setzen
            if (g_keyboardView && g_keyboardView.glView) {
                [g_keyboardView.glView setFrame:[[g_window contentView] bounds]];
                [g_keyboardView.glView setNeedsDisplay:YES];
            }
            g_fullscreen = true;
            printf("[MAC] Fullscreen: ON, Size: %u x %u\n", g_window_width, g_window_height);
            fflush(stdout);
        } else {
            // Restore windowed mode mit Decorations
            NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
            [g_window setStyleMask:style];
            [g_window setLevel:NSNormalWindowLevel];
            [g_window setBackgroundColor:[NSColor blackColor]];
            [g_window setOpaque:YES];
            // Menüleiste und Dock wiederherstellen
            [NSApp setPresentationOptions:NSApplicationPresentationDefault];
            NSScreen* screen = [g_window screen] ?: [NSScreen mainScreen];
            NSRect screenFrame = [screen frame];
            CGFloat window_x = screenFrame.origin.x + (screenFrame.size.width - Config::VIEWPORT_WIDTH) / 2;
            CGFloat window_y = screenFrame.origin.y + (screenFrame.size.height - Config::VIEWPORT_HEIGHT) / 2;
            NSRect windowedFrame = NSMakeRect(window_x, window_y, Config::VIEWPORT_WIDTH, Config::VIEWPORT_HEIGHT);
            [g_window setFrame:windowedFrame display:YES animate:NO];
            g_window_width = Config::VIEWPORT_WIDTH;
            g_window_height = Config::VIEWPORT_HEIGHT;
            // OpenGLView auf neue Größe setzen
            if (g_keyboardView && g_keyboardView.glView) {
                [g_keyboardView.glView setFrame:[[g_window contentView] bounds]];
                [g_keyboardView.glView setNeedsDisplay:YES];
            }
            g_fullscreen = false;
            printf("[MAC] Fullscreen: OFF, Size: %u x %u\n", g_window_width, g_window_height);
            fflush(stdout);
        }
    }
}

bool is_fullscreen()
{
    return g_fullscreen;
}

bool alt_down()
{
    // On Mac, Option key is ALT equivalent
    return ([NSEvent modifierFlags] & NSEventModifierFlagOption) != 0;
}

}
