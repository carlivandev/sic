
#pragma once

#include <msdfgen.h>
#include <msdfgen-ext.h>
#include "types.h"
#include "GlyphBox.h"

namespace msdf_atlas {

/// Represent's the shape geometry of a single glyph as well as its configuration
class GlyphGeometry {

public:
    GlyphGeometry();
    /// Loads glyph geometry from font
    bool load(msdfgen::FontHandle *font, unicode_t codepoint);
    /// Applies edge coloring to glyph shape
    void edgeColoring(double angleThreshold, unsigned long long seed);
    /// Computes the dimensions of the glyph's box as well as the transformation for the generator function
    void wrapBox(double scale, double range, double miterLimit);
    /// Sets the glyph's box's position in the atlas
    void placeBox(int x, int y);
    /// Returns the glyph's Unicode index
    unicode_t getCodepoint() const;
    /// Returns the glyph's shape
    const msdfgen::Shape & getShape() const;
    /// Returns the glyph's advance
    double getAdvance() const;
    /// Returns true if the shape has reverse winding
    bool isWindingReverse() const;
    /// Outputs the position and dimensions of the glyph's box in the atlas
    void getBoxRect(int &x, int &y, int &w, int &h) const;
    /// Outputs the dimensions of the glyph's box in the atlas
    void getBoxSize(int &w, int &h) const;
    /// Returns the range needed to generate the glyph's SDF
    double getBoxRange() const;
    /// Returns the scale needed to generate the glyph's bitmap
    double getBoxScale() const;
    /// Returns the translation vector needed to generate the glyph's bitmap
    msdfgen::Vector2 getBoxTranslate() const;
    /// Outputs the bounding box of the glyph as it should be placed on the baseline
    void getQuadPlaneBounds(double &l, double &b, double &r, double &t) const;
    /// Outputs the bounding box of the glyph in the atlas
    void getQuadAtlasBounds(double &l, double &b, double &r, double &t) const;
    /// Returns true if the glyph is a whitespace and has no geometry
    bool isWhitespace() const;
    /// Simplifies to GlyphBox
    operator GlyphBox() const;

    msdfgen::Shape::Bounds bounds;
private:
    unicode_t codepoint;
    msdfgen::Shape shape;
    bool reverseWinding;
    double advance;
    struct {
        struct {
            int x, y, w, h;
        } rect;
        double range;
        double scale;
        msdfgen::Vector2 translate;
    } box;

    /// Computes the signed distance from point p in a naive way
    double simpleSignedDistance(const msdfgen::Point2 &p) const;

};

}