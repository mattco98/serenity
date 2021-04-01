test("basic functionality", () => {
    let s = new Set([1, 2, 3]);
    expect(s.has(1)).toBeTrue();
    expect(s.has(2)).toBeTrue();
    expect(s.has(3)).toBeTrue();
    expect(s.has(4)).toBeFalse();
});
