import QtQuick
//import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Basic

Item {
    id: root
    required property string text
    required property string author
    required property double timestampTicks
    property var usersModel: null
    property bool hideUsernames: false
    property string currentUsername: ""
    height: implicitHeight
    readonly property bool isWide: width > 700
    readonly property bool isMineMessage:
        author.length > 0
        && currentUsername.length > 0
        && author.toLowerCase() === currentUsername.toLowerCase()
    Item {
        id: limitter
        width: Math.min(400, parent.width)
        height: parent.height
        opacity: 0.2
    }

    readonly property string authorDisplayLine: {
        let _reloadTick = root.usersModel ? root.usersModel.loadStatus : 0
        let login = root.author || ""
        let nick = ""
        if (root.usersModel && typeof root.usersModel.displayNameForUsername === "function") {
            nick = root.usersModel.displayNameForUsername(login) || ""
        }
        if (root.hideUsernames) {
            if (nick.length > 0)
                return nick
            return login.length > 0 ? login : "unknown"
        }
        let base = login.length > 0 ? login : "unknown"
        if (nick.length > 0 && nick.toLowerCase() !== login.toLowerCase())
            return nick + " @" + base
        return base
    }

    readonly property var messageDate: timestampTicks > 0 ? new Date(timestampTicks * 1000) : null
    readonly property var nowDate: new Date()
    readonly property bool isToday: messageDate !== null
        && messageDate.getFullYear() === nowDate.getFullYear()
        && messageDate.getMonth() === nowDate.getMonth()
        && messageDate.getDate() === nowDate.getDate()
    readonly property string formattedTimestamp: timestampTicks > 0
        ? (isToday
            ? Qt.formatDateTime(messageDate, "HH:mm")
            : Qt.formatDateTime(messageDate, "dd.MM.yyyy HH:mm:ss"))
        : ""

    implicitHeight: msgColumn.implicitHeight + 12
    Rectangle {
        id: messagePane
        readonly property bool mineToRight: !root.isWide && root.isMineMessage
        //anchors.left: mineToRight ? undefined : parent.left
        //anchors.right: mineToRight ? parent.right : undefined
        x: mineToRight ? parent.width - width : 0
        width: msgColumn.width + msgColumn.x
        height: msgColumn.height + msgColumn.anchors.topMargin + msgColumn.anchors.bottomMargin
        radius: 12
        bottomLeftRadius: mineToRight ? radius : 2
        bottomRightRadius: mineToRight ? 2 : radius

        Column {
            id: msgColumn
            spacing: 2

            Label {
                text: root.authorDisplayLine
                color: "#9aa4b2"
                font.pixelSize: 11
                elide: Label.ElideRight
                leftPadding: messagePane.topLeftRadius
                rightPadding: messagePane.topRightRadius
            }
            Item {
                width: messageText.width
                height: messageText.height
                Text {
                    id: measureText
                    padding: messageText.padding
                    leftPadding: messageText.leftPadding
                    rightPadding: messageText.rightPadding
                    text: messageText.text
                    visible: false
                    width: limitter.width
                    textFormat: messageText.textFormat
                    wrapMode: Text.Wrap
                }

                Text {
                    id: messageText
                    padding: 8
                    width: Math.min(limitter.width, measureText.contentWidth)
                    text: root.text || "(empty message)"
                    wrapMode: Text.Wrap
                    textFormat: Text.MarkdownText
                    onLinkActivated: link => Qt.openUrlExternally(link)
                    ToolTip {
                        text: messageText.hoveredLink
                        visible: !!text
                        delay: 1000
                    }
                }
            }
        }
    }
}
