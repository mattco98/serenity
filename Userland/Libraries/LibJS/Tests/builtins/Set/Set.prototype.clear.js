test("basic functionality", () => {
    let s = new Set([1, 2, 3]);
    s.clear();
    expect(s.size).toBe(0);
    s.clear();
    expect(s.size).toBe(0);
});

test.skip("clear while iterating", () => {
    let s = new Set([1, 2, 3]);
    let it = s[Symbol.iterator]();

    expect(it).toNextIterateTo(false, 1);
    expect(it).toNextIterateTo(false, 2);
    s.clear();
    expect(it).toNextIterateTo(true);
});
