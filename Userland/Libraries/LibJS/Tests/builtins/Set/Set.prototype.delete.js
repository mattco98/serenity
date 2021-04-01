test("basic functionality", () => {
    let s = new Set([1, 2, 3]);
    expect(s.delete(2)).toBeTrue();
    expect(s.delete(10)).toBeFalse();

    expect(s.has(2)).toBeFalse();
    expect(s.has(10)).toBeFalse();
    expect(s.size).toBe(2);
});

test.skip("delete value already iterated", () => {
    let s = new Set([1, 2, 3]);
    let it = s[Symbol.iterator]();

    expect(it).toNextIterateTo(false, 1);
    expect(it).toNextIterateTo(false, 2);
    expect(s.delete(1)).toBeTrue();
    expect(s.delete(2)).toBeTrue();
    expect(it).toNextIterateTo(false, 3);
    expect(it).toNextIterateTo(true);

});

test.skip("delete value not yet iterated", () => {
    let s = new Set([1, 2, 3]);
    let it = s[Symbol.iterator]();

    expect(it).toNextIterateTo(false, 1);
    expect(s.delete(2)).toBeTrue();
    expect(it).toNextIterateTo(false, 3);
    expect(it).toNextIterateTo(true);
});
