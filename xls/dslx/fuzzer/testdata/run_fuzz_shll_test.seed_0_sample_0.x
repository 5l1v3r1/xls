type x21 = uN[0x4];
type x23 = uN[0x3d];fn main(x0: u48, x1: s18, x2: u10, x3: u63, x4: u44, x5: u20, x6: u61) -> (uN[0x20], u1) {
    let x7: uN[0x3a] = (x2) ++ (x0);
    let x8: u61 = clz(x6);
    let x9: u1 = or_reduce(x6);
    let x10: u63 = -(x3);
    let x11: s18 = for (i, x): (u4, s18) in range((u4:0x0), (u4:0x0)) {
    x
  }(x1)
  ;
    let x12: u1 = xor_reduce(x0);
    let x13: uN[0x3f] = x10;
    let x14: u1 = and_reduce(x6);
    let x15: (s18, u1, u20, u44, u20, u1, u63, s18, u20, u1, u10, u1, u20, s18, u1) = (x11, x9, x5, x4, x5, x12, x3, x1, x5, x9, x2, x14, x5, x11, x12);
    let x16: u10 = for (i, x): (u4, u10) in range((u4:0x0), (u4:0x4)) {
    x
  }(x2)
  ;
    let x17: s18 = (x15)[(u32:0xd)];
    let x18: u1 = or_reduce(x8);
    let x19: u1 = clz(x14);
    let x20: x21[0xb] = (x4 as x21[0xb]);
    let x22: x23[0x1] = (x8 as x23[0x1]);
    let x24: uN[0x15] = one_hot(x5, (u1:0x1));
    let x25: u1 = (x15)[(u32:0x5)];
    let x26: uN[0xb] = (x17)[-0x12:-0x7];
    let x27: u1 = !(x25);
    let x28: uN[0x20] = (x0)[x12+:uN[0x20]];
    let x29: u61 = (x6) << (((u61:0x16)) if (((x4 as u61)) >= ((u61:0x16))) else ((x4 as u61)));
    let x30: u1 = xor_reduce(x18);
    let x31: u1 = for (i, x): (u4, u1) in range((u4:0x0), (u4:0x2)) {
    x
  }(x14)
  ;
    let x32: u1 = one_hot_sel(x31, [x9]);
    (x28, x32)
}