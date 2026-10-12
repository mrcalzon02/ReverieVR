package io.github.mrcalzon02.reverievr;

import java.util.ArrayList;
import java.util.Map;
import java.util.TreeMap;

/**
 * Stable per-device controller selection. Android InputDevice IDs are
 * connection-session values; persist descriptors instead. Duplicate or
 * missing descriptors are ambiguous and intentionally not selectable.
 */
final class ControllerDeviceSelection {
    private ControllerDeviceSelection() {}

    static String[] selectable(String[] descriptors) {
        TreeMap<String, Integer> counts = new TreeMap<>();
        if (descriptors != null) for (String descriptor : descriptors) {
            if (descriptor != null && !descriptor.trim().isEmpty())
                counts.put(descriptor, counts.containsKey(descriptor)
                    ? counts.get(descriptor) + 1 : 1);
        }
        ArrayList<String> unique = new ArrayList<>();
        for (Map.Entry<String, Integer> e : counts.entrySet())
            if (e.getValue() == 1) unique.add(e.getKey());
        return unique.toArray(new String[0]);
    }

    static String retainOrFirst(String[] selectable, String current) {
        if (selectable == null || selectable.length == 0) return "";
        if (contains(selectable, current)) return current;
        return selectable[0];
    }

    static String next(String[] selectable, String current) {
        if (selectable == null || selectable.length == 0) return "";
        for (int i = 0; i < selectable.length; i++)
            if (selectable[i].equals(current))
                return selectable[(i + 1) % selectable.length];
        return selectable[0];
    }

    static boolean accepts(String selected, String incoming,
                           String[] selectable) {
        return selected != null && !selected.isEmpty()
            && selected.equals(incoming) && contains(selectable, selected);
    }

    static boolean contains(String[] selectable, String candidate) {
        if (selectable == null || candidate == null || candidate.isEmpty())
            return false;
        for (String item : selectable) if (candidate.equals(item)) return true;
        return false;
    }
}
