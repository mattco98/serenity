test("basic functionality", () => {
    let s = new Set();
    expect(s.add(1)).toBe(s);
    expect(s.add(2)).toBe(s);
    expect(s.add(s)).toBe(s);

    expect(s.has(1)).toBeTrue();
    expect(s.has(2)).toBeTrue();
    expect(s.has(s)).toBeTrue();
});

test("-0 converted to +0", () => {
    let s = new Set();
    s.add(0);
    s.add(-0);
    expect(s.size).toBe(1);
});

test.skip("add while iterating", () => {
    let s = new Set([1, 2]);
    let it = s[Symbol.iterator]();

    expect(it).toNextIterateTo(false, 1);
    s.add(10);
    expect(it).toNextIterateTo(false, 2);
    expect(it).toNextIterateTo(false, 10);
    expect(it).toNextIterateTo(true);
});
