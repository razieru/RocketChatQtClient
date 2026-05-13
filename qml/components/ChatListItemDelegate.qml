import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    required property int index
    required property string chatId
    required property string name
    required property string type
    required property int unread
    required property string section
    property var onChatClicked

    color: ListView.isCurrentItem ? "#1f2a44" : "transparent"
    border.color: ListView.isCurrentItem ? "#4d78ff" : "#333333"
    border.width: 1
    radius: 6

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (root.onChatClicked) {
                root.onChatClicked(root.chatId)
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Label {
            text: root.name
            Layout.fillWidth: true
            elide: Label.ElideRight
        }
        Label { text: root.type.length > 0 ? root.type : "-" }
        Label { text: root.unread > 0 ? ("Unread: " + root.unread) : "" }
    }
}
