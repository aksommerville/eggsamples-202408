/**
 * Provides some conveniences related to text.
 *  - Global language selection.
 *  - Decode UTF-8.
 *  - String resource cache and decoding.
 *  - Access to "metadata" resource.
 */
export class TextService {
  private language = 0; // Resource qualifier, or zero if not initialized yet.
  private availableLanguages: number[] = []; // Empty only if uninitialized.
  private strings: string[] = []; // Sparse, keyed by (rid). Empty string if known absent.
  private metadata: { [key: string]: string; } | null = null;
  
  constructor() {
    this.language = this.guessLanguage();
  }
  
  getLanguage(): number {
    return this.language;
  }
  
  setLanguage(lang: number): boolean {
    if (!lang) return false;
    if (lang === this.language) return false;
    this.language = lang;
    this.strings = [];
    return true;
  }
  
  getString(rid: number): string {
    let str = this.strings[rid];
    if (typeof(str) === "string") return str;
    const serial = egg.res_get(egg.ResType.string, this.language, rid);
    str = serial ? this.decodeString(serial) : "";
    this.strings[rid] = str;
    return str;
  }
  
  getQualifiedString(qual: number, rid: number): string {
    if (qual === this.language) return this.getString(rid);
    const serial = egg.res_get(egg.ResType.string, qual, rid);
    if (!serial) return "";
    return this.decodeString(serial);
  }
  
  decodeString(serial: ArrayBuffer): string {
    const src = new Uint8Array(serial);
    let dst = "";
    for (let srcp=0; srcp<src.length; ) {
      let codepoint=0, seqlen=0;
      if (!(src[srcp] & 0x80)) {
        codepoint = src[srcp];
        seqlen = 1;
      } else if (!(src[srcp] & 0x40)) {
      } else if (!(src[srcp] & 0x20)) {
        if ((src[srcp+1] & 0xc0) === 0x80) {
          codepoint = ((src[srcp] & 0x1f) << 6) | (src[srcp+1] & 0x3f);
          seqlen = 2;
        }
      } else if (!(src[srcp] & 0x10)) {
        if (((src[srcp+1] & 0xc0) === 0x80) && ((src[srcp+2] & 0xc0) === 0x80)) {
          codepoint = ((src[srcp] & 0x0f) << 12) | ((src[srcp+1] & 0x3f) << 6) | (src[srcp+2] & 0x3f);
          seqlen = 3;
        }
      } else if (!(src[srcp] & 0x08)) {
        if (((src[srcp+1] & 0xc0) === 0x80) && ((src[srcp+2] & 0xc0) === 0x80) && ((src[srcp+3] & 0xc0) === 0x80)) {
          codepoint = ((src[srcp] & 0x07) << 18) | ((src[srcp+1] & 0x3f) << 12) | ((src[srcp+2] & 0x3f) << 6) | (src[srcp+3] & 0x3f);
          seqlen = 4;
        }
      }
      if (!seqlen) {
        // Any misencode, pass thru one raw byte as a codepoint.
        codepoint = src[srcp];
        seqlen = 1;
      }
      dst += String.fromCharCode(codepoint);
      srcp += seqlen;
    }
    return dst;
  }
  
  decodeStringUint8Array(src: Uint8Array): string {
    const tmp = new Uint8Array(src.length);
    tmp.set(src);
    return this.decodeString(tmp.buffer);
  }
  
  /**
   * All unique language codes discovered among our string resources.
   */
  getAvailableLanguages(): number[] {
    if (!this.availableLanguages.length) {
      let lastQual = 0;
      for (let p=0; ; p++) {
        const res = egg.res_id_by_index(p);
        if (!res || !res.tid) break;
        if (res.tid > egg.ResType.string) break;
        if (res.tid < egg.ResType.string) continue;
        if (res.qual === lastQual) continue;
        lastQual = res.qual;
        this.availableLanguages.push(res.qual);
      }
      if (!this.availableLanguages.length) {
        // Game does not contain any string resources. Insert a dummy so we know it's been checked.
        this.availableLanguages.push(0x7a7a); // "zz": Legal but not assigned.
      }
    }
    return this.availableLanguages;
  }
  
  /**
   * Same as getAvailableLanguages, but also include string:x:1 for each, which 
   * conventionally is the language's name in itself.
   */
  getLanguagesWithDescription(): [number, string][] {
    return this.getAvailableLanguages().map(lang => [lang, this.getQualifiedString(lang, 1)]);
  }
  
  evalLanguage(src: string): number {
    src = src.trim().toLowerCase();
    if (src.length !== 2) return 0;
    const a = src.charCodeAt(0);
    const b = src.charCodeAt(1);
    if ((a < 0x61) || (a > 0x7a) || (b < 0x61) || (b > 0x7a)) return 0;
    return (a << 8) | b;
  }
  
  /**
   * Decode resource metadata:0:1 if needed, and fetch a string from it.
   * If (translate), the default, we'll look first for "${key}Str" and use that string resource instead,
   * in the current global language.
   */
  getMetadataField(key: string, translate = true): string {
    if (!this.metadata) {
      this.metadata = {};
      const serial = egg.res_get(egg.ResType.metadata, 0, 1);
      if (serial) {
        this.decodeMetadata(this.metadata, serial);
      }
    }
    if (translate) {
      const strid = this.metadata[key + "Str"];
      if (strid) {
        const str = this.getString(+strid);
        if (str) return str;
      }
    }
    return this.metadata[key] || "";
  }
  
  private decodeMetadata(dst: { [key: string]: string; }, serial: ArrayBuffer): void {
    const src = new Uint8Array(serial);
    for (let srcp=0; srcp<src.length; ) {
      const kc = src[srcp++] || 0;
      const vc = src[srcp++] || 0;
      if (srcp + kc + vc > src.length) break;
      const k = this.decodeStringUint8Array(src.slice(srcp, srcp + kc));
      srcp += kc;
      const v = this.decodeStringUint8Array(src.slice(srcp, srcp + vc));
      srcp += vc;
      dst[k] = v;
    }
  }
  
  private guessLanguage(): number {
    const userPrefs = egg.get_user_languages();
    const available = this.getAvailableLanguages();
    // userPrefs is prioritized; available is not.
    for (const lang of userPrefs) {
      if (available.indexOf(lang) >= 0) return lang;
    }
    // Read the metadata "language" header if present, it's prioritized.
    // Since no user language is supported, we should prefer the game's original language.
    // (as opposed to available[0], which would just be the lowest language alphabetically).
    let fallback = 0;
    const declared = this.getMetadataField("language");
    for (const langstr of declared.split(',')) {
      const lang = this.evalLanguage(langstr);
      if (!lang) continue;
      if (available.indexOf(lang) >= 0) return lang; // Declared and present: Use it.
      if (!fallback) fallback = lang; // Declared but no strings. Huh? Use the first like this, if nothing good matches.
    }
    if (fallback) return fallback;
    // Finally, use the first available language. This array is never empty; it will contain "zz" if no strings are present.
    return available[0];
  }
}
