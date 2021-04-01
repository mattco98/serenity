test.skip("basic functionality", () => {
    let s = new Set([1, 2, 3]);
    let values = s.values();
    expect(values).toNextIterateTo(false, 1);
    expect(values).toNextIterateTo(false, 2);
    expect(values).toNextIterateTo(false, 3);
    expect(values).toNextIterateTo(true);
});

test.skip("delete value not yet iterated", () => {
    let s = new Set([1, 2, 3]);
    let values = s.values();
    expect(values).toNextIterateTo(false, 1);
    expect(s.delete(2)).toBeTrue();
    expect(values).toNextIterateTo(false, 3);
    expect(values).toNextIterateTo(true);
});
