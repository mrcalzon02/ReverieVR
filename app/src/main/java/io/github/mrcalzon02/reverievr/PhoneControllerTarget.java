package io.github.mrcalzon02.reverievr;

final class PhoneControllerTarget {
    final String address;
    final String displayName;

    PhoneControllerTarget(String address, String displayName) {
        this.address = address == null ? "" : address;
        this.displayName =
            displayName == null || displayName.trim().isEmpty()
                ? "Paired Android device"
                : displayName;
    }

    String label() {
        return displayName + "\n" + address;
    }
}
