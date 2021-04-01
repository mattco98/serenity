test.skip("basic functionality", () => {
    let s = new Set([1, 2, 3]);
    let keys = s.keys();
    expect(keys).toNextIterateTo(false, 1);
    expect(keys).toNextIterateTo(false, 2);
    expect(keys).toNextIterateTo(false, 3);
    expect(keys).toNextIterateTo(true);
});

test.skip("delete value not yet iterated", () => {
    let s = new Set([1, 2, 3]);
    let keys = s.keys();
    expect(keys).toNextIterateTo(false, 1);
    expect(s.delete(2)).toBeTrue();
    expect(keys).toNextIterateTo(false, 3);
    expect(keys).toNextIterateTo(true);
});
