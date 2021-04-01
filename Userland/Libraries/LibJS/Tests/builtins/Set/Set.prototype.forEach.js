test("basic functionality", () => {
    let s = new Set();
    let orderedValues = [];

    for (let i = 0; i < 100; i++) {
        let v = Math.random();
        if (s.has(v)) {
            // On the off change that Math.random produces a duplicate result
            continue;
        }

        s.add(v);
        orderedValues.push(v);
    }

    let i = 0;
    s.forEach((v1, v2, set) => {
        expect(set).toBe(s);
        expect(v1).toBe(v2);
        expect(v1).toBe(orderedValues[i++]);
    });
});

test.skip("delete property before iterating", () => {
    let s = new Set([1, 2, 3]);

    let i = 0;
    s.forEach(v => {
        if (i === 0) {
            expect(v).toBe(1);
            s.delete(2);
        } else if (i === 1) {
            expect(v).toBe(3);
        } else {
            expect.fail("should not have iterated more than two times");
        }
        i++;
    });
});
