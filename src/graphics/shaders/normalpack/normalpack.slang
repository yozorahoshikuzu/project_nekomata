float2 octahedronWrap(float2 v) {
    return (1.0 - abs(v.yx)) * select(v.xy >= 0.0, 1.0, -1.0);
}

float2 octahedronNormalPack(float3 n) {
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0 ? n.xy : octahedronWrap(n.xy);
    return n.xy * 0.5 + 0.5;
}

float3 octahedronNormalUnpack(float2 pn) {
    pn = pn * 2.0 - 1.0;
    float3 n = float3(pn.x, pn.y, 1.0 - abs(pn.x) - abs(pn.y));
    float t = saturate(-n.z);
    n.xy += select(n.xy >= 0.0, -t, t);
    return normalize(n);
}