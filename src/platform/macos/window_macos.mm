#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#include <iostream>

@interface AetherWaveView : NSView
@property (nonatomic, assign) std::vector<float>* bands;
@property (nonatomic, assign) int mode; // 0 = Bars, 1 = Waveform Curve, 2 = Mirrored
@end

@implementation AetherWaveView
- (void)drawRect:(NSRect)dirtyRect {
    [[NSColor clearColor] set];
    NSRectFill(dirtyRect);

    if (!self.bands || self.bands->empty()) return;

    size_t count = self.bands->size();
    CGFloat width = self.bounds.size.width;
    CGFloat height = self.bounds.size.height;

    if (self.mode == 1) {
        // Waveform Curve Mode — strictly synced with Equalizer Bars speed (Zero temporal lag!)
        NSBezierPath* curvePath = [NSBezierPath bezierPath];
        CGFloat step_x = width / (count - 1);
        CGFloat baseline_y = 3.0;
        CGFloat max_amp = height * 0.88;

        std::vector<CGFloat> curve_heights(count);
        for (size_t i = 0; i < count; ++i) {
            CGFloat prev = (i > 0) ? (*self.bands)[i - 1] : (*self.bands)[i];
            CGFloat curr = (*self.bands)[i];
            CGFloat next = (i + 1 < count) ? (*self.bands)[i + 1] : (*self.bands)[i];
            curve_heights[i] = prev * 0.15 + curr * 0.70 + next * 0.15;
            if (i < 3) curve_heights[i] *= ((CGFloat)i / 3.0);
            else if (i >= count - 3) curve_heights[i] *= ((CGFloat)(count - 1 - i) / 3.0);
        }

        [curvePath moveToPoint:NSMakePoint(0, baseline_y + curve_heights[0] * max_amp)];
        for (size_t i = 0; i < count - 1; ++i) {
            CGFloat x1 = i * step_x;
            CGFloat y1 = baseline_y + curve_heights[i] * max_amp;
            CGFloat x2 = (i + 1) * step_x;
            CGFloat y2 = baseline_y + curve_heights[i + 1] * max_amp;
            NSPoint cp1 = NSMakePoint(x1 + (x2 - x1) / 2.0, y1);
            NSPoint cp2 = NSMakePoint(x1 + (x2 - x1) / 2.0, y2);
            [curvePath curveToPoint:NSMakePoint(x2, y2) controlPoint1:cp1 controlPoint2:cp2];
        }

        [[NSColor colorWithRed:0.55 green:0.36 blue:0.96 alpha:0.95] setStroke];
        [curvePath setLineWidth:2.5];
        [curvePath stroke];

        // Fill down to baseline
        [curvePath lineToPoint:NSMakePoint(width, 0)];
        [curvePath lineToPoint:NSMakePoint(0, 0)];
        [curvePath closePath];
        [[NSColor colorWithRed:0.55 green:0.36 blue:0.96 alpha:0.25] setFill];
        [curvePath fill];
        return;
    }

    CGFloat spacing = 3.0;
    CGFloat bar_width = (width - (count - 1) * spacing) / count;

    for (size_t i = 0; i < count; ++i) {
        CGFloat h = (*self.bands)[i] * (height - 8.0);
        NSRect barRect = NSMakeRect(i * (bar_width + spacing), 0, bar_width, h);

        CGFloat t = (CGFloat)i / (count - 1);
        // Crimson to Blue interpolation
        NSColor* barColor = [NSColor colorWithRed:(1.0 - t * 0.9)
                                            green:(0.1 + t * 0.5)
                                             blue:(0.3 + t * 0.7)
                                            alpha:0.92];
        [barColor set];
        NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:barRect xRadius:2.5 yRadius:2.5];
        [path fill];
    }
}
@end

class MacOSWindowManager {
public:
    MacOSWindowManager() : m_window(nil), m_view(nil) {}

    bool Create(CGFloat height_px) {
        NSScreen* screen = [NSScreen mainScreen];
        NSRect screenFrame = [screen frame];
        NSRect visibleFrame = [screen visibleFrame];

        // Dock strictly above macOS Dock with safety gap
        CGFloat pos_y = visibleFrame.origin.y + 2.0;

        NSRect windowFrame = NSMakeRect(screenFrame.origin.x, pos_y,
                                        screenFrame.size.width, height_px);

        m_window = [[NSWindow alloc] initWithContentRect:windowFrame
                                               styleMask:NSWindowStyleMaskBorderless
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];

        [m_window setOpaque:NO];
        [m_window setBackgroundColor:[NSColor clearColor]];
        [m_window setIgnoresMouseEvents:YES]; // 100% Click-through
        [m_window setLevel:kCGStatusWindowLevel]; // Above normal windows
        [m_window setCollectionBehavior:(NSWindowCollectionBehaviorCanJoinAllSpaces |
                                         NSWindowCollectionBehaviorStationary)];

        m_view = [[AetherWaveView alloc] initWithFrame:NSMakeRect(0, 0, screenFrame.size.width, height_px)];
        [m_window setContentView:m_view];
        [m_window orderFrontRegardless];

        return true;
    }

    void UpdateBars(std::vector<float>& bands, int mode = 0) {
        if (!m_view) return;
        m_view.bands = &bands;
        m_view.mode = mode;
        [m_view setNeedsDisplay:YES];
    }

    void Destroy() {
        if (m_window) {
            [m_window close];
            m_window = nil;
        }
    }

private:
    NSWindow* m_window;
    AetherWaveView* m_view;
};
#endif
