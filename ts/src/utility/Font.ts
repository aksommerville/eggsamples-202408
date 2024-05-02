interface FontImage {
  v: ArrayBuffer;
  src: Uint8Array; // view into (this.v)
  w: number;
  h: number;
  stride: number;
  fmt: egg.TextureFormat;
}

interface FontGlyph {
  x: number;
  y: number;
  w: number;
  // (h) is constant across the Font.
}

interface FontPage {
  codepoint: number;
  glyphs: FontGlyph[];
  image: FontImage;
}

/**
 * Generates text images.
 * Source text is UTF-8, not limited to G0+G1.
 * Output has a fixed row height and variable width.
 * Source images must be 1-bit, with a blank column between glyphs.
 * It is not possible for individual glyphs to contain horizontal space (eg U+22 QUOTE, it has to look weird).
 * Empty glyphs within an image are not possible. You have to add something, say a dot, to skip it.
 * Images must use qualifier zero.
 *
 * To set up a Font you give us three things:
 *  - Row height.
 *  - First codepoint of each page.
 *  - Image ID of each page.
 */
export class Font {
  private pages: FontPage[] = [];
  private rowh: number;
  private spacew: number;
  
  constructor(
    rowh: number
  ) {
    if (rowh < 1) throw new Error(`Invalid row height`);
    this.rowh = rowh;
    this.spacew = this.rowh >> 1;
  }
  
  getRowHeight(): number {
    return this.rowh;
  }
  
  /**
   * Call directly after construction, once for each page of glyphs you want to use.
   */
  addPage(codepoint: number, imageId: number): boolean {
    const hdr = egg.image_get_header(0, imageId);
    if (!hdr || !hdr.w) return false;
    switch (hdr.fmt) {
      case egg.TextureFormat.A1:
      case egg.TextureFormat.Y1:
        break;
      default: return false;
    }
    const v = egg.image_decode(0, imageId);
    if (!v) return false;
    const page: FontPage = {
      image: {
        v,
        src: new Uint8Array(v),
        w: hdr.w,
        h: hdr.h,
        fmt: hdr.fmt,
        stride: Math.floor(v.byteLength / hdr.h),
      },
      codepoint,
      glyphs: [],
    };
    if (!this.slicePage(page)) return false;
    if (!this.insertPage(page)) return false;
    return true;
  }
  
  /**
   * Allocate a new texture and render some text to it.
   * Returns 0 on errors.
   * After success, you must eventually free it with egg.texture_del().
   */
  renderTexture(src: string, wlimit=0, color=0xffffff): number {
    const image = this.render(src, wlimit, color);
    const texid = egg.texture_new();
    if (texid < 1) return 0;
    if (egg.texture_upload(texid, image.w, image.h, image.stride, image.fmt, image.v) < 0) {
      egg.texture_del(texid);
      return 0;
    }
    return texid;
  }
  
  /**
   * Render some text to a new RGBA image.
   * If you supply (wlimit), we produce exactly that width and break lines as needed.
   * Otherwise it will be one line, exactly as wide as it needs to be.
   */
  render(src: string, wlimit=0, color=0xffffff): FontImage {
    let lines: string[] = [];
    if (wlimit < 1) {
      wlimit = this.measure(src);
      if (wlimit < 1) wlimit = 1;
      lines.push(src);
    } else {
      lines = this.breakLines(src, wlimit);
      if (!lines.length) lines.push("");
    }
    const dst = this.createImage(wlimit, lines.length * this.rowh);
    for (let linei=0, y=0; linei<lines.length; linei++, y+=this.rowh) {
      this.renderLine(dst, 0, y, lines[linei], color);
    }
    return dst;
  }
  
  /**
   * Return the width of this string if it were rendered in one line.
   */
  measure(src: string): number {
    let w=0;
    for (let i=0; i<src.length; i++) {
      const ch = src.charCodeAt(i);
      w += this.getGlyphWidth(ch);
      w++;
    }
    if (w) w--; // Remove the last post-glyph space.
    return w;
  }
  
  /**
   * Split (src) on newlines, or wherever its rendered width would exceed (wlimit).
   * Leading space and spaces immediately after a newline are preserved, other space may be chucked.
   * We try to split between words, as you'd expect.
   */
  breakLines(src: string, wlimit: number): string[] {
    const dst: string[] = []; // Finished lines only.
    let linep=0, srcp=0, linew=0;
    while (srcp < src.length) {
    
      // At a newline, break and do not consume indentation.
      if (src[srcp] === '\n') {
        let endp = srcp;
        while ((endp > linep) && (src.charCodeAt(endp - 1) <= 0x20)) endp--;
        dst.push(src.substring(linep, endp));
        srcp++;
        linep = srcp;
        linew = 0;
        continue;
      }
    
      // Measure the next word.
      const nextp = this.nextBreakPoint(src, srcp);
      const nextw = this.measure(src.substring(srcp, nextp));
      
      // Fits? Great, keep it.
      if (linew + nextw <= wlimit) {
        linew += nextw;
        srcp = nextp;
        continue;
      }
      
      // Got some content already? Commit it, skip whitespace, and try again.
      if (linep < srcp) {
        let endp = srcp;
        while ((endp > linep) && (src.charCodeAt(endp - 1) <= 0x20)) endp--;
        dst.push(src.substring(linep, endp));
        linep = srcp;
        linew = 0;
        while ((linep < src.length) && (src.charCodeAt(linep) <= 0x20) && (src[linep] !== '\n')) linep++;
        continue;
      }
      
      // Break midword: Emit as many characters as fit, and at least one even if it overflows.
      linep = srcp + 1;
      linew = this.measure(src.substring(srcp, linep));
      while (linew < wlimit) {
        const maybew = this.measure(src.substring(srcp, linep + 1));
        if (maybew > wlimit) break;
        linep++;
        linew = maybew;
      }
      let endp = srcp;
      while ((endp > linep) && (src.charCodeAt(endp - 1) <= 0x20)) endp--;
      dst.push(src.substring(srcp, endp));
      srcp = linep;
      linew = 0;
    }
    if (linep < srcp) {
      let endp = srcp;
      while ((endp > linep) && (src.charCodeAt(endp - 1) <= 0x20)) endp--;
      if (linep < endp) dst.push(src.substring(linep, endp));
    }
    return dst;
  }
  
  /**
   * Measure space then non-space, stopping at the next block of spaces.
   */
  nextBreakPoint(src: string, p: number): number {
    while ((p < src.length) && (src.charCodeAt(p) <= 0x20)) p++;
    while ((p < src.length) && (src.charCodeAt(p) > 0x20)) p++;
    return p;
  }
  
  /**
   * Render one line of text, with the first glyph's top left corner at (x,y).
   * (dst) must be RGBA.
   */
  renderLine(dst: FontImage, x: number, y: number, src: string, color=0xffffff): void {
    for (let srci=0; srci<src.length; srci++) {
      const ch = src.charCodeAt(srci);
      if ((ch === 0x20) || (ch === 0xa0)) {
        x += this.spacew;
        continue;
      }
      if (ch === 0x09) {
        x += this.rowh;
        continue;
      }
      const page = this.pageForCodepoint(ch);
      if (!page) continue;
      const glyph = page.glyphs[ch - page.codepoint];
      this.blitRgbaA1(dst, x, y, page.image, glyph.x, glyph.y, glyph.w, this.rowh, color);
      x += glyph.w;
      x += 1;
    }
  }
  
  /**
   * Render one glyph centered on the given point.
   */
  renderGlyphCentered(dst: FontImage, x: number, y: number, ch: number, color=0xffffff): void {
    if ((ch === 0x20) || (ch === 0xa0) || (ch === 0x09)) return;
    const page = this.pageForCodepoint(ch);
    if (!page) return;
    const glyph = page.glyphs[ch - page.codepoint];
    x -= glyph.w >> 1;
    y -= this.rowh >> 1;
    this.blitRgbaA1(dst, x, y, page.image, glyph.x, glyph.y, glyph.w, this.rowh, color);
  }
  
  /**
   * Generate a new RGBA image suitable for rendering to.
   */
  createImage(w: number, h: number): FontImage {
    const stride = w << 2;
    const src = new Uint8Array(stride * h);
    return {
      fmt: egg.TextureFormat.RGBA,
      w, h, stride,
      v: src.buffer,
      src,
    };
  }
  
  getGlyphWidth(codepoint: number): number {
    // Space, HT, and NBSP are special. They don't have glyphs but they do have width.
    if ((codepoint === 0x20) || (codepoint === 0xa0)) return this.spacew;
    if (codepoint === 0x09) return this.rowh;
    const page = this.pageForCodepoint(codepoint);
    if (!page) return 0;
    return page.glyphs[codepoint - page.codepoint].w;
  }
  
  pageForCodepoint(codepoint: number): FontPage | null {
    let lo=0, hi=this.pages.length;
    while (lo < hi) {
      const ck = (lo + hi) >> 1;
      const q = this.pages[ck];
           if (codepoint < q.codepoint) hi = ck;
      else if (codepoint >= q.codepoint + q.glyphs.length) lo = ck + 1;
      else return q;
    }
    return null;
  }
  
  private blitRgbaA1(
    dst: FontImage,
    dstx: number, dsty: number,
    src: FontImage,
    srcx: number, srcy: number,
    w: number, h: number,
    rgb: number
  ): void {
    const r = (rgb >> 16) & 0xff;
    const g = (rgb >> 8) & 0xff;
    const b = rgb & 0xff;
    let dstrowp = dsty * dst.stride + (dstx << 2);
    let srcrowp = srcy * src.stride + (srcx >> 3);
    const srcmask0 = 0x80 >> (srcx & 7);
    for (let yi=h; yi-->0; dstrowp+=dst.stride, srcrowp+=src.stride) {
      let dstp = dstrowp;
      let srcp = srcrowp;
      let srcmask = srcmask0;
      for (let xi=w; xi-->0; dstp+=4) {
        if (src.src[srcp] & srcmask) {
          dst.src[dstp] = r;
          dst.src[dstp+1] = g;
          dst.src[dstp+2] = b;
          dst.src[dstp+3] = 0xff;
        }
        if (srcmask === 1) { srcmask = 0x80; srcp++; }
        else srcmask >>= 1;
      }
    }
  }
  
  private slicePage(page: FontPage): boolean {
    for (let y=0; y<page.image.h; y+=this.rowh) {
      for (let x=0;; ) {
        x += this.measureSpace(page.image, x, y);
        if (x >= page.image.w) break;
        const w = this.measureGlyph(page.image, x, y);
        if (w < 1) return false;
        page.glyphs.push({ x, y, w });
        x += w;
      }
    }
    return true;
  }
  
  private measureSpace(image: FontImage, x: number, y: number): number {
    const x0 = x;
    for (; x<image.w; x++) {
      if (!this.columnIsBlank(image, x, y)) break;
    }
    return x - x0;
  }
  
  private measureGlyph(image: FontImage, x: number, y: number): number {
    const x0 = x;
    for (; x<image.w; x++) {
      if (this.columnIsBlank(image, x, y)) break;
    }
    return x - x0;
  }
  
  private columnIsBlank(image: FontImage, x: number, y: number): boolean {
    const mask = 0x80 >> (x & 7);
    let srcp = y * image.stride + (x >> 3);
    let yi = this.rowh;
    for (; yi-->0; srcp+=image.stride) {
      if (image.src[srcp] & mask) return false;
    }
    return true;
  }
  
  private insertPage(page: FontPage): boolean {
    if (page.glyphs.length < 1) return false;
    let lo=0, hi=this.pages.length;
    while (lo < hi) {
      const ck = (lo + hi) >> 1;
      const q = this.pages[ck];
           if (page.codepoint < q.codepoint) hi = ck;
      else if (page.codepoint > q.codepoint) lo = ck + 1;
      else return false;
    }
    if ((lo > 0) && (page.codepoint < this.pages[lo-1].codepoint + this.pages[lo-1].glyphs.length)) return false;
    if ((lo < this.pages.length) && (page.codepoint + page.glyphs.length > this.pages[lo].codepoint)) return false;
    this.pages.splice(lo, 0, page);
    return true;
  }
}
