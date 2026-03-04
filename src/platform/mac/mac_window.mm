#define GL_SILENCE_DEPRECATION
#import "mac_window.h"
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

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
        [g_window makeKeyAndOrderFront:nil];

        NSOpenGLPixelFormatAttribute attrs[] = {
            NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
            NSOpenGLPFAColorSize, 24,
            NSOpenGLPFAAlphaSize, 8,
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFADepthSize, 24,
            0
        };

        NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        g_glContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
        [g_glContext makeCurrentContext];

        // Set swap interval (vsync)
        GLint swapInt = 1;
        [g_glContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

        // Enable basic GL state
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        return true;
    }
}

void swap_buffers()
{
    @autoreleasepool {
        [g_glContext flushBuffer];
    }
}

void clear_screen()
{
    glClear(GL_COLOR_BUFFER_BIT);
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
                [NSApp updateWindows];
                if ([event type] == NSEventTypeApplicationDefined) {
                    g_shouldClose = true;
                }
            }
        } while (event != nil);

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

} // namespace Platform
