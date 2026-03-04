#define GL_SILENCE_DEPRECATION
#import "mac_window.h"
#include "renderer/renderer.h"
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

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
static bool g_shouldClose = false;

bool init_window(const WindowConfig& config)
{
    @autoreleasepool {
        [NSApplication sharedApplication];

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
        [glView setWantsBestResolutionOpenGLSurface:YES];
        
        // Get the context from the view and make it current
        g_glContext = [glView openGLContext];
        [g_glContext makeCurrentContext];

        // Set swap interval (vsync)
        GLint swapInt = 1;
        [g_glContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

        // Set the OpenGL view as the window's content view
        [g_window setContentView:glView];

        // Set up window delegate for close events
        WindowDelegate* delegate = [[WindowDelegate alloc] init];
        delegate.shouldClosePtr = &g_shouldClose;
        [g_window setDelegate:delegate];

        [NSApp finishLaunching];
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

}
