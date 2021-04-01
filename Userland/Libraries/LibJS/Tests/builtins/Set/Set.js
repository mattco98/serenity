const verifySetHas = (set, values) => {
    values.forEach(value => {
        expect(set.has(value)).toBeTrue();
    });
    expect(set.size).toBe(values.length)
};

test("constructor properties", () => {
    expect(Set).toHaveLength(1);
    expect(Set.name).toBe("Set");
});

test("constructor with string", () => {
    let s = new Set("hello");
    expect(s.has("h")).toBeTrue();
    expect(s.has("e")).toBeTrue();
    expect(s.has("l")).toBeTrue();
    expect(s.has("o")).toBeTrue();
    expect(s.size).toBe(4);
});

test("constructor with array", () => {
    let s = new Set([1, 2, 3]);
    expect(s.has(1)).toBeTrue();
    expect(s.has(2)).toBeTrue();
    expect(s.has(3)).toBeTrue();
    expect(s.size).toBe(3);
});

test.skip("constructor with other set", () => {
    let s = new Set(new Set([1, 2, 3]));
    expect(s.has(1)).toBeTrue();
    expect(s.has(2)).toBeTrue();
    expect(s.has(3)).toBeTrue();
    expect(s.size).toBe(3);
});
