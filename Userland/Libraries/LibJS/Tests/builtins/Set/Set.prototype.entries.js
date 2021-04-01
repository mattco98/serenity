test.skip("basic functionality", () => {
    let s = new Set([1, 2, 3]);
    let entries = s.entries();
    expect(entries).toNextIterateTo(false, [1, 1]);
    expect(entries).toNextIterateTo(false, [2, 2]);
    expect(entries).toNextIterateTo(false, [3, 3]);
    expect(entries).toNextIterateTo(true);
});

test.skip("delete value not yet iterated", () => {
    let s = new Set([1, 2, 3]);
    let entries = s.entries();
    expect(entries).toNextIterateTo(false, [1, 1]);
    expect(s.delete(2)).toBeTrue();
    expect(entries).toNextIterateTo(false, [3, 3]);
    expect(entries).toNextIterateTo(true);
});
