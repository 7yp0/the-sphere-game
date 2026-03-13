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
}

- (void)drawRect:(NSRect)dirtyRect {
    // Let GL view handle rendering
    [self.glView display];
}

- (void)keyDown:(NSEvent *)event {
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
    return g_window_width;
}

uint32_t get_window_height()
{
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

}
