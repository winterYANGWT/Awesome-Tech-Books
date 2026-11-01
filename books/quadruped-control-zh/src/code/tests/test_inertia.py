"""Whole-body equivalent inertia used by the balance controller (Ch. 6)."""

from __future__ import annotations

import numpy as np

from quadruped.robot import A1Params


def test_body_equivalent_inertia_matches_menagerie_whole_body():
    params = A1Params.from_menagerie()
    I = params.body_equivalent_inertia(params.home_q)

    # Chapter 6.3: sum the 13 Menagerie bodies about the whole-body COM.
    assert np.allclose(np.diag(I), [0.141070, 0.367186, 0.399641], atol=1e-4)
    assert np.allclose(I, I.T)
    assert np.all(np.linalg.eigvalsh(I) > 0)
    # It is the whole-body value, not the trunk-only tensor.
    assert np.all(np.diag(I) > 7.0 * np.diag(params.trunk_inertia))

    # The folded-leg value must depend on the standing configuration.
    q = params.home_q.copy()
    q[2] += 0.2
    assert not np.allclose(params.body_equivalent_inertia(q), I, atol=1e-4)
