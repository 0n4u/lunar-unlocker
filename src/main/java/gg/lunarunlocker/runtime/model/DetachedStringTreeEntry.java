package gg.lunarunlocker.runtime.model;

import gg.lunarunlocker.runtime.model.DetachedStringTreeNode;

class DetachedStringTreeEntry {
    String value;
    final DetachedStringTreeNode ownerNode;

    DetachedStringTreeEntry(DetachedStringTreeNode ownerNode) {
        this.ownerNode = ownerNode;
    }
}
